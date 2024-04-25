#include "ut/ut.h"
#include "ut/win32.h"

// TODO: test FILE_APPEND_DATA
// https://learn.microsoft.com/en-us/windows/win32/fileio/appending-one-file-to-another-file?redirectedfrom=MSDN

// are posix and Win32 seek in agreement?
static_assertion(SEEK_SET == FILE_BEGIN);
static_assertion(SEEK_CUR == FILE_CURRENT);
static_assertion(SEEK_END == FILE_END);

#ifndef O_SYNC
#define O_SYNC (0x10000)
#endif

static errno_t files_open(file_t* *file, const char* fn, int32_t f) {
    DWORD access = (f & files.o_wr) ? GENERIC_WRITE :
                   (f & files.o_rw) ? GENERIC_READ | GENERIC_WRITE :
                                      GENERIC_READ;
    access |= (f & files.o_append) ? FILE_APPEND_DATA : 0;
    DWORD disposition =
        (f & files.o_create) ? ((f & files.o_excl)  ? CREATE_NEW :
                                (f & files.o_trunc) ? CREATE_ALWAYS :
                                                      OPEN_ALWAYS) :
            (f & files.o_trunc) ? TRUNCATE_EXISTING : OPEN_EXISTING;
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    DWORD attr = FILE_ATTRIBUTE_NORMAL;
    attr |= (f & O_SYNC) ? FILE_FLAG_WRITE_THROUGH : 0;
    *file = CreateFileA(fn, access, share, null, disposition, attr, null);
    return *file != INVALID_HANDLE_VALUE ? 0 : runtime.err();
}

static bool files_is_valid(file_t* file) { // both null and files.invalid
    return file != files.invalid && file != null;
}

static errno_t files_seek(file_t* file, int64_t *position, int32_t method) {
    LARGE_INTEGER distance_to_move = { .QuadPart = *position };
    LARGE_INTEGER pointer = {0};
    errno_t r = b2e(SetFilePointerEx(file, distance_to_move, &pointer, method));
    if (r == 0) { *position = pointer.QuadPart; };
    return r;
}

static inline uint64_t files_ft_to_us(FILETIME ft) { // us (microseconds)
    return (ft.dwLowDateTime | (((uint64_t)ft.dwHighDateTime) << 32)) / 10;
}

static int64_t files_a2t(DWORD a) {
    int64_t type = 0;
    if (a & FILE_ATTRIBUTE_REPARSE_POINT) {
        type |= files.type_symlink;
    }
    if (a & FILE_ATTRIBUTE_DIRECTORY) {
        type |= files.type_folder;
    }
    if (a & FILE_ATTRIBUTE_DEVICE) {
        type |= files.type_device;
    }
    return type;
}

#ifdef FILES_LINUX_PATH_BY_FD

static int get_final_path_name_by_fd(int fd, char *buffer, int32_t bytes) {
    swear(bytes >= 0);
    char fd_path[16 * 1024];
    // /proc/self/fd/* is a symbolic link
    snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd);
    size_t len = readlink(fd_path, buffer, bytes - 1);
    if (len != -1) { buffer[len] = 0x00; } // Null-terminate the result
    return len == -1 ? errno : 0;
}

#endif

static errno_t files_stat(file_t* file, files_stat_t* s, bool follow_symlink) {
    errno_t r = 0;
    BY_HANDLE_FILE_INFORMATION fi;
    fatal_if_false(GetFileInformationByHandle(file, &fi));
    const bool symlink = (fi.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    if (follow_symlink && symlink) {
        const DWORD flags = FILE_NAME_NORMALIZED | VOLUME_NAME_DOS;
        DWORD n = GetFinalPathNameByHandleA(file, null, 0, flags);
        if (n == 0) {
            r = GetLastError();
        } else {
            char* name = null;
            r = heap.allocate(null, &name, n + 2, false);
            if (r == 0) {
                n = GetFinalPathNameByHandleA(file, name, n + 1, flags);
                if (n == 0) {
                    r = GetLastError();
                } else {
                    file_t* f = files.invalid;
                    r = files.open(&f, name, files.o_rd);
                    if (r == 0) { // keep following:
                        r = files.stat(f, s, follow_symlink);
                        files.close(f);
                    }
                }
                heap.deallocate(null, name);
            }
        }
    } else {
        s->size = fi.nFileSizeLow | (((uint64_t)fi.nFileSizeHigh) << 32);
        s->created  = files_ft_to_us(fi.ftCreationTime); // since epoch
        s->accessed = files_ft_to_us(fi.ftLastAccessTime);
        s->updated  = files_ft_to_us(fi.ftLastWriteTime);
        s->type = files_a2t(fi.dwFileAttributes);
    }
    return r;
}

static errno_t files_read(file_t* file, void* data, int64_t bytes, int64_t *transferred) {
    errno_t r = 0;
    *transferred = 0;
    while (bytes > 0 && r == 0) {
        DWORD chunk_size = (DWORD)(bytes > UINT32_MAX ? UINT32_MAX : bytes);
        DWORD bytes_read = 0;
        r = b2e(ReadFile(file, data, chunk_size, &bytes_read, null));
        if (r == 0) {
            *transferred += bytes_read;
            bytes -= bytes_read;
            data = (uint8_t*)data + bytes_read;
        }
    }
    return r;
}

static errno_t files_write(file_t* file, const void* data, int64_t bytes, int64_t *transferred) {
    errno_t r = 0;
    *transferred = 0;
    while (bytes > 0 && r == 0) {
        DWORD chunk_size = (DWORD)(bytes > UINT32_MAX ? UINT32_MAX : bytes);
        DWORD bytes_read = 0;
        r = b2e(WriteFile(file, data, chunk_size, &bytes_read, null));
        if (r == 0) {
            *transferred += bytes_read;
            bytes -= bytes_read;
            data = (uint8_t*)data + bytes_read;
        }
    }
    return r;
}

static errno_t files_flush(file_t* file) {
    return b2e(FlushFileBuffers(file));
}

static void files_close(file_t* file) {
    fatal_if_false(CloseHandle(file));
}

static errno_t files_write_fully(const char* filename, const void* data,
                                 int64_t bytes, int64_t *transferred) {
    if (transferred != null) { *transferred = 0; }
    errno_t r = 0;
    const DWORD access = GENERIC_WRITE;
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const DWORD flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH;
    HANDLE file = CreateFileA(filename, access, share, null, CREATE_ALWAYS,
                              flags, null);
    if (file == INVALID_HANDLE_VALUE) {
        r = runtime.err();
    } else {
        int64_t written = 0;
        const uint8_t* p = (const uint8_t*)data;
        while (r == 0 && bytes > 0) {
            uint64_t write = bytes >= UINT32_MAX ?
                (UINT32_MAX) - 0xFFFF : (uint64_t)bytes;
            assert(0 < write && write < UINT32_MAX);
            DWORD chunk = 0;
            r = b2e(WriteFile(file, p, (DWORD)write, &chunk, null));
            written += chunk;
            bytes -= chunk;
        }
        if (transferred != null) { *transferred = written; }
        errno_t rc = b2e(CloseHandle(file));
        if (r == 0) { r = rc; }
    }
    return r;
}

static errno_t files_unlink(const char* pathname) {
    if (files.is_folder(pathname)) {
        return b2e(RemoveDirectoryA(pathname));
    } else {
        return b2e(DeleteFileA(pathname));
    }
}

static errno_t files_create_tmp(char* fn, int32_t count) {
    // create temporary file (not folder!) see folders_test() about racing
    swear(fn != null && count > 0);
    const char* tmp = files.tmp();
    errno_t r = 0;
    if (count < (int)strlen(tmp) + 8) {
        r = ERROR_BUFFER_OVERFLOW;
    } else {
        assert(count > (int)strlen(tmp) + 8);
        // If GetTempFileNameA() succeeds, the return value is the length,
        // in chars, of the string copied to lpBuffer, not including the
        // terminating null character.If the function fails,
        // the return value is zero.
        if (count > (int)strlen(tmp) + 8) {
            char prefix[4] = {0};
            r = GetTempFileNameA(tmp, prefix, 0, fn) == 0 ? runtime.err() : 0;
            if (r == 0) {
                assert(files.exists(fn) && !files.is_folder(fn));
            } else {
                traceln("GetTempFileNameA() failed %s", str.error(r));
            }
        } else {
            r = ERROR_BUFFER_OVERFLOW;
        }
    }
    return r;
}

#pragma push_macro("files_acl_args")
#pragma push_macro("files_get_acl")
#pragma push_macro("files_set_acl")

#define files_acl_args(acl) DACL_SECURITY_INFORMATION, null, null, acl, null

#define files_get_acl(obj, type, acl, sd)                       \
    (type == SE_FILE_OBJECT ? GetNamedSecurityInfoA((char*)obj, \
             SE_FILE_OBJECT, files_acl_args(acl), &sd) :        \
    (type == SE_KERNEL_OBJECT) ? GetSecurityInfo((HANDLE)obj,   \
             SE_KERNEL_OBJECT, files_acl_args(acl), &sd) :      \
    ERROR_INVALID_PARAMETER)

#define files_set_acl(obj, type, acl)                           \
    (type == SE_FILE_OBJECT ? SetNamedSecurityInfoA((char*)obj, \
             SE_FILE_OBJECT, files_acl_args(acl)) :             \
    (type == SE_KERNEL_OBJECT) ? SetSecurityInfo((HANDLE)obj,   \
             SE_KERNEL_OBJECT, files_acl_args(acl)) :           \
    ERROR_INVALID_PARAMETER)

static errno_t files_acl_add_ace(ACL* acl, SID* sid, uint32_t mask,
                                 ACL** free_me, byte flags) {
    ACL_SIZE_INFORMATION info;
    ACL* bigger = null;
    uint32_t bytes_needed = sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(sid)
                          - sizeof(DWORD);
    errno_t r = b2e(GetAclInformation(acl, &info, sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation));
    if (r == 0 && info.AclBytesFree < bytes_needed) {
        const int64_t bytes = info.AclBytesInUse + bytes_needed;
        r = heap.allocate(null, &bigger, bytes, true);
        if (r == 0) {
            r = b2e(InitializeAcl((ACL*)bigger,
                    info.AclBytesInUse + bytes_needed, ACL_REVISION));
        }
    }
    if (r == 0 && bigger != null) {
        for (int32_t i = 0; i < (int)info.AceCount; i++) {
            ACCESS_ALLOWED_ACE* ace;
            r = b2e(GetAce(acl, i, (void**)&ace));
            if (r != 0) { break; }
            r = b2e(AddAce(bigger, ACL_REVISION, MAXDWORD, ace,
                           ace->Header.AceSize));
            if (r != 0) { break; }
        }
    }
    if (r == 0) {
        ACCESS_ALLOWED_ACE* ace = null;
        r = heap.allocate(null, &ace, bytes_needed, true);
        if (r == 0) {
            ace->Header.AceFlags = flags;
            ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
            ace->Header.AceSize = (WORD)bytes_needed;
            ace->Mask = mask;
            ace->SidStart = sizeof(ACCESS_ALLOWED_ACE);
            memcpy(&ace->SidStart, sid, GetLengthSid(sid));
            r = b2e(AddAce(bigger != null ? bigger : acl, ACL_REVISION, MAXDWORD,
                           ace, bytes_needed));
            heap.deallocate(null, ace);
        }
    }
    *free_me = bigger;
    return r;
}

static errno_t files_lookup_sid(ACCESS_ALLOWED_ACE* ace) {
    // handy for debugging
    SID* sid = (SID*)&ace->SidStart;
    DWORD l1 = 128, l2 = 128;
    char account[128];
    char group[128];
    SID_NAME_USE use;
    errno_t r = b2e(LookupAccountSidA(null, sid, account,
                                     &l1, group, &l2, &use));
    if (r == 0) {
        traceln("%s/%s: type: %d, mask: 0x%X, flags:%d",
                group, account,
                ace->Header.AceType, ace->Mask, ace->Header.AceFlags);
    } else {
        traceln("LookupAccountSidA() failed %s", str.error(r));
    }
    return r;
}

static errno_t files_add_acl_ace(const void* obj, int32_t obj_type,
                                 int32_t sid_type, uint32_t mask) {
    uint8_t stack[SECURITY_MAX_SID_SIZE];
    DWORD n = countof(stack);
    SID* sid = (SID*)stack;
    errno_t r = b2e(CreateWellKnownSid((WELL_KNOWN_SID_TYPE)sid_type,
                                       null, sid, &n));
    if (r != 0) {
        return ERROR_INVALID_PARAMETER;
    }
    ACL* acl = null;
    void* sd = null;
    r = files_get_acl(obj, obj_type, &acl, sd);
    if (r == 0) {
        ACCESS_ALLOWED_ACE* found = null;
        for (int32_t i = 0; i < acl->AceCount; i++) {
            ACCESS_ALLOWED_ACE* ace;
            r = b2e(GetAce(acl, i, (void**)&ace));
            if (r != 0) { break; }
            if (EqualSid((SID*)&ace->SidStart, sid)) {
                if (ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE &&
                   (ace->Header.AceFlags & INHERITED_ACE) == 0) {
                    found = ace;
                } else if (ace->Header.AceType !=
                           ACCESS_ALLOWED_ACE_TYPE) {
                    traceln("%d ACE_TYPE is not supported.",
                             ace->Header.AceType);
                    r = ERROR_INVALID_PARAMETER;
                }
                break;
            }
        }
        if (r == 0 && found) {
            if ((found->Mask & mask) != mask) {
//              traceln("updating existing ace");
                found->Mask |= mask;
                r = files_set_acl(obj, obj_type, acl);
            } else {
//              traceln("desired access is already allowed by ace");
            }
        } else if (r == 0) {
//          traceln("inserting new ace");
            ACL* new_acl = null;
            byte flags = obj_type == SE_FILE_OBJECT ?
                CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE : 0;
            r = files_acl_add_ace(acl, sid, mask, &new_acl, flags);
            if (r == 0) {
                r = files_set_acl(obj, obj_type, (new_acl != null ? new_acl : acl));
            }
            if (new_acl != null) { heap.deallocate(null, new_acl); }
        }
    }
    if (sd != null) { LocalFree(sd); }
    return r;
}

#pragma pop_macro("files_set_acl")
#pragma pop_macro("files_get_acl")
#pragma pop_macro("files_acl_args")

static errno_t files_chmod777(const char* pathname) {
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID everyone = null; // Create a well-known SID for the Everyone group.
    fatal_if_false(AllocateAndInitializeSid(&SIDAuthWorld, 1,
             SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &everyone));
    EXPLICIT_ACCESSA ea[1] = {{0}};
    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    ea[0].grfAccessPermissions = 0xFFFFFFFF;
    ea[0].grfAccessMode  = GRANT_ACCESS; // The ACE will allow everyone all access.
    ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPSTR)everyone;
    // Create a new ACL that contains the new ACEs.
    ACL* acl = null;
    fatal_if_not_zero(SetEntriesInAclA(1, ea, null, &acl));
    // Initialize a security descriptor.
    uint8_t stack[SECURITY_DESCRIPTOR_MIN_LENGTH];
    SECURITY_DESCRIPTOR* sd = (SECURITY_DESCRIPTOR*)stack;
    fatal_if_false(InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION));
    // Add the ACL to the security descriptor.
    fatal_if_false(SetSecurityDescriptorDacl(sd, /* DaclPresent flag: */ true,
                                   acl, /* not a default DACL: */  false));
    // Change the security attributes
    errno_t r = b2e(SetFileSecurityA(pathname, DACL_SECURITY_INFORMATION, sd));
    if (r != 0) {
        traceln("chmod777(%s) failed %s", pathname, str.error(r));
    }
    if (everyone != null) { FreeSid(everyone); }
    if (acl != null) { LocalFree(acl); }
    return r;
}

// https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createdirectorya
// "If lpSecurityAttributes is null, the directory gets a default security
//  descriptor. The ACLs in the default security descriptor for a directory
//  are inherited from its parent directory."

static errno_t files_mkdirs(const char* dir) {
    const int32_t n = (int)strlen(dir) + 1;
    char* s = null;
    errno_t r = heap.allocate(null, &s, n, true);
    const char* next = strchr(dir, '\\');
    if (next == null) { next = strchr(dir, '/'); }
    while (r == 0 && next != null) {
        if (next > dir && *(next - 1) != ':') {
            memcpy(s, dir, next - dir);
            r = b2e(CreateDirectoryA(s, null));
            if (r == ERROR_ALREADY_EXISTS) { r = 0; }
        }
        if (r == 0) {
            const char* prev = ++next;
            next = strchr(prev, '\\');
            if (next == null) { next = strchr(prev, '/'); }
        }
    }
    if (r == 0) {
        r = b2e(CreateDirectoryA(dir, null));
    }
    heap.deallocate(null, s);
    return r == ERROR_ALREADY_EXISTS ? 0 : r;
}

#pragma push_macro("files_realloc_path")
#pragma push_macro("files_append_name")

#define files_realloc_path(r, pn, pnc, fn, name) do {                   \
    const int32_t bytes = (int32_t)(strlen(fn) + strlen(name) + 3);     \
    if (bytes > pnc) {                                                  \
        r = heap.reallocate(null, &pn, bytes, false);                   \
        if (r != 0) {                                                   \
            pnc = bytes;                                                \
        } else {                                                        \
            heap.deallocate(null, pn);                                  \
            pn = null;                                                  \
        }                                                               \
    }                                                                   \
} while (0)

#define files_append_name(pn, pnc, fn, name) do {      \
    if (strequ(fn, "\\") || strequ(fn, "/")) {         \
        str.format(pn, pnc, "\\%s", name);            \
    } else {                                           \
        str.format(pn, pnc, "%.*s\\%s", k, fn, name); \
    }                                                  \
} while (0)

static errno_t files_rmdirs(const char* fn) {
    files_stat_t st;
    folder_t folder;
    errno_t r = files.opendir(&folder, fn);
    if (r == 0) {
        int32_t k = (int32_t)strlen(fn);
        // remove trailing backslash (except if it is root: "/" or "\\")
        if (k > 1 && (fn[k - 1] == '/' || fn[k - 1] == '\\')) {
            k--;
        }
        int32_t pnc = 64 * 1024; // pathname "pn" capacity in bytes
        char* pn = null;
        r = heap.allocate(null, &pn, pnc, false);
        while (r == 0) {
            // recurse into sub folders and remove them first
            // do NOT follow symlinks - it could be disastrous
            const char* name = files.readdir(&folder, &st);
            if (name == null) { break; }
            if (!strequ(name, ".") && !strequ(name, "..") &&
                (st.type & files.type_symlink) == 0 &&
                (st.type & files.type_folder) != 0) {
                files_realloc_path(r, pn, pnc, fn, name);
                if (r == 0) {
                    files_append_name(pn, pnc, fn, name);
                    r = files.rmdirs(pn);
                }
            }
        }
        files.closedir(&folder);
        r = files.opendir(&folder, fn);
        while (r == 0) {
            const char* name = files.readdir(&folder, &st);
            if (name == null) { break; }
            // symlinks are already removed as normal files
            if (!strequ(name, ".") && !strequ(name, "..") &&
                (st.type & files.type_folder) == 0) {
                files_realloc_path(r, pn, pnc, fn, name);
                if (r == 0) {
                    files_append_name(pn, pnc, fn, name);
                    r = files.unlink(pn);
                    if (r != 0) {
                        traceln("remove(%s) failed %s", pn, str.error(r));
                    }
                }
            }
        }
        heap.deallocate(null, pn);
        files.closedir(&folder);
    }
    if (r == 0) { r = files.unlink(fn); }
    return r;
}

#pragma pop_macro("files_append_name")
#pragma pop_macro("files_realloc_path")

static bool files_exists(const char* path) {
    return PathFileExistsA(path);
}

static bool files_is_folder(const char* path) {
    return PathIsDirectoryA(path);
}

static bool files_is_symlink(const char* filename) {
    DWORD attributes = GetFileAttributesA(filename);
    return attributes != INVALID_FILE_ATTRIBUTES &&
          (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

const char* files_basename(const char* pathname) {
    const char* bn = strrchr(pathname, '\\');
    if (bn == null) { bn = strrchr(pathname, '/'); }
    return bn != null ? bn + 1 : pathname;
}

static errno_t files_copy(const char* s, const char* d) {
    return b2e(CopyFileA(s, d, false));
}

static errno_t files_move(const char* s, const char* d) {
    static const DWORD flags =
        MOVEFILE_REPLACE_EXISTING |
        MOVEFILE_COPY_ALLOWED |
        MOVEFILE_WRITE_THROUGH;
    return b2e(MoveFileExA(s, d, flags));
}

static errno_t files_link(const char* from, const char* to) {
    // note reverse order of parameters:
    return b2e(CreateHardLinkA(to, from, null));
}

static errno_t files_symlink(const char* from, const char* to) {
    // The correct order of parameters for CreateSymbolicLinkA is:
    // CreateSymbolicLinkA(symlink_to_create, existing_file, flags);
    DWORD flags = files.is_folder(from) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
    return b2e(CreateSymbolicLinkA(to, from, flags));
}

static const char* files_bin(void) {
    static char program_files[files_max_path];
    if (program_files[0] == 0) {
        wchar_t* program_files_w = null;
        fatal_if(SHGetKnownFolderPath(&FOLDERID_ProgramFilesX64, 0,
            null, &program_files_w) != 0);
        int32_t len = (int32_t)wcslen(program_files_w);
        assert(len < countof(program_files));
        fatal_if(len >= countof(program_files), "len=%d", len);
        for (int32_t i = 0; i <= len; i++) { // including zero terminator
            assert(program_files_w[i] < 128); // pure ascii
            program_files[i] = (char)program_files_w[i];
        }
    }
    return program_files;
}

static const char* files_tmp(void) {
    static char tmp[files_max_path];
    if (tmp[0] == 0) {
        // If GetTempPathA() succeeds, the return value is the length,
        // in chars, of the string copied to lpBuffer, not including
        // the terminating null character. If the function fails, the
        // return value is zero.
        errno_t r = GetTempPathA(countof(tmp), tmp) == 0 ? runtime.err() : 0;
        fatal_if(r != 0, "GetTempPathA() failed %s", str.error(r));
    }
    return tmp;
}

static errno_t files_cwd(char* fn, int32_t count) {
    swear(count > 1);
    DWORD bytes = count - 1;
    errno_t r = b2e(GetCurrentDirectoryA(bytes, fn));
    fn[count - 1] = 0; // always
    return r;
}

static errno_t files_chdir(const char* fn) {
    return b2e(SetCurrentDirectoryA(fn));
}



typedef struct files_dir_s {
    HANDLE handle;
    WIN32_FIND_DATAA find; // On Win64: 320 bytes
} files_dir_t;

static_assertion(sizeof(files_dir_t) <= sizeof(folder_t));

errno_t files_opendir(folder_t* folder, const char* folder_name) {
    files_dir_t* d = (files_dir_t*)folder;
    int32_t n = (int32_t)strlen(folder_name);
    char* fn = null;
    errno_t r = heap.allocate(null, &fn, n + 3, false); // extra room for "\*" suffix
    if (r == 0) {
        snprintf(fn, n + 3, "%s\\*", folder_name);
        fn[n + 2] = 0;
        d->handle = FindFirstFileA(fn, &d->find);
        if (d->handle == INVALID_HANDLE_VALUE) { r = GetLastError(); }
        heap.deallocate(null, fn);
    }
    return r;
}

static uint64_t files_ft2us(FILETIME* ft) { // 100ns units to microseconds:
    return (((uint64_t)ft->dwHighDateTime) << 32 | ft->dwLowDateTime) / 10;
}

const char* files_readdir(folder_t* folder, files_stat_t* s) {
    const char* fn = null;
    files_dir_t* d = (files_dir_t*)folder;
    if (FindNextFileA(d->handle, &d->find)) {
        fn = d->find.cFileName;
        // Ensure zero termination
        d->find.cFileName[countof(d->find.cFileName) - 1] = 0x00;
        if (s != null) {
            s->accessed = files_ft2us(&d->find.ftLastAccessTime);
            s->created = files_ft2us(&d->find.ftCreationTime);
            s->updated = files_ft2us(&d->find.ftLastWriteTime);
            s->type = files_a2t(d->find.dwFileAttributes);
            s->size = (((uint64_t)d->find.nFileSizeHigh) << 32) |
                                  d->find.nFileSizeLow;
        }
    }
    return fn;
}

void files_closedir(folder_t* folder) {
    files_dir_t* d = (files_dir_t*)folder;
    fatal_if_false(FindClose(d->handle));
}

#pragma push_macro("files_test_failed")

#ifdef RUNTIME_TESTS

#define files_test_failed " failed %s", str.error(runtime.err())

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                 \
    if (debug.verbosity.level >= debug.verbosity.trace) { \
        traceln(__VA_ARGS__);                             \
    }                                                     \
} while (0)

static void folders_dump_time(const char* label, uint64_t us) {
    int32_t year = 0;
    int32_t month = 0;
    int32_t day = 0;
    int32_t hh = 0;
    int32_t mm = 0;
    int32_t ss = 0;
    int32_t ms = 0;
    int32_t mc = 0;
    clock.local(us, &year, &month, &day, &hh, &mm, &ss, &ms, &mc);
    traceln("%-7s: %04d-%02d-%02d %02d:%02d:%02d.%03d:%03d",
            label, year, month, day, hh, mm, ss, ms, mc);
}

static void folders_test(void) {
    uint64_t now = clock.microseconds(); // microseconds since epoch
    uint64_t before = now - 1 * clock.usec_in_sec; // one second earlier
    uint64_t after  = now + 2 * clock.usec_in_sec; // two seconds later
    int32_t year = 0;
    int32_t month = 0;
    int32_t day = 0;
    int32_t hh = 0;
    int32_t mm = 0;
    int32_t ss = 0;
    int32_t ms = 0;
    int32_t mc = 0;
    clock.local(now, &year, &month, &day, &hh, &mm, &ss, &ms, &mc);
    verbose("now: %04d-%02d-%02d %02d:%02d:%02d.%03d:%03d",
             year, month, day, hh, mm, ss, ms, mc);
    // Test cwd, setcwd
    const char* tmp = files.tmp();
    char cwd[256] = {0};
    fatal_if(files.cwd(cwd, sizeof(cwd)) != 0, "files.cwd() failed");
    fatal_if(files.chdir(tmp) != 0, "files.chdir(\"%s\") failed %s",
                tmp, str.error(runtime.err()));
    // there is no racing free way to create temporary folder
    // without having a temporary file for the duration of folder usage:
    char tmp_file[files_max_path]; // create_tmp() is thread safe race free:
    errno_t r = files.create_tmp(tmp_file, countof(tmp_file));
    fatal_if(r != 0, "files.create_tmp() failed %s", str.error(r));
    char tmp_dir[files_max_path];
    strprintf(tmp_dir, "%s.dir", tmp_file);
    r = files.mkdirs(tmp_dir);
    fatal_if(r != 0, "files.mkdirs(%s) failed %s", tmp_dir, str.error(r));
    verbose("%s", tmp_dir);
    folder_t folder;
    char pn[files_max_path] = {0};
    strprintf(pn, "%s/file", tmp_dir);
    // cannot test symlinks because they are only
    // available to Administrators and in Developer mode
//  char sym[files_max_path] = {0};
    char hard[files_max_path] = {0};
    char sub[files_max_path] = {0};
    strprintf(hard, "%s/hard", tmp_dir);
    strprintf(sub, "%s/subd", tmp_dir);
    const char* content = "content";
    int64_t transferred = 0;
    r = files.write_fully(pn, content, strlen(content), &transferred);
    fatal_if(r != 0, "files.write_fully(\"%s\") failed %s", pn, str.error(r));
    swear(transferred == (int64_t)strlen(content));
    r = files.link(pn, hard);
    fatal_if(r != 0, "files.link(\"%s\", \"%s\") failed %s",
                      pn, hard, str.error(r));
    r = files.mkdirs(sub);
    fatal_if(r != 0, "files.mkdirs(\"%s\") failed %s", sub, str.error(r));
    r = files.opendir(&folder, tmp_dir);
    fatal_if(r != 0, "files.opendir(\"%s\") failed %s", tmp_dir, str.error(r));
    for (;;) {
        files_stat_t st = {0};
        const char* name = files.readdir(&folder, &st);
        if (name == null) { break; }
        uint64_t at = st.accessed;
        uint64_t ct = st.created;
        uint64_t ut = st.updated;
        swear(ct <= at && ct <= ut);
        clock.local(ct, &year, &month, &day, &hh, &mm, &ss, &ms, &mc);
        bool is_folder = st.type & files.type_folder;
        bool is_symlink = st.type & files.type_symlink;
        int64_t bytes = st.size;
        verbose("%s: %04d-%02d-%02d %02d:%02d:%02d.%03d:%03d %lld bytes %s%s",
                name, year, month, day, hh, mm, ss, ms, mc,
                bytes, is_folder ? "[folder]" : "", is_symlink ? "[symlink]" : "");
        if (strequ(name, "file") || strequ(name, "hard")) {
            swear(bytes == (int64_t)strlen(content),
                    "size of \"%s\": %lld is incorrect expected: %d",
                    name, bytes, transferred);
        }
        if (strequ(name, ".") || strequ(name, "..")) {
            swear(is_folder, "\"%s\" is_folder: %d", name, is_folder);
        } else {
            swear(strequ(name, "subd") == is_folder,
                  "\"%s\" is_folder: %d", name, is_folder);
            // empirically timestamps are imprecise on NTFS
            swear(at >= before, "access: %lld  >= %lld", at, before);
            if (ct < before || ut < before || at >= after || ct >= after || ut >= after) {
                traceln("file: %s", name);
                folders_dump_time("before", before);
                folders_dump_time("create", ct);
                folders_dump_time("update", ut);
                folders_dump_time("access", at);
            }
            swear(ct >= before, "create: %lld  >= %lld", ct, before);
            swear(ut >= before, "update: %lld  >= %lld", ut, before);
            // and no later than 2 seconds since folders_test()
            swear(at < after, "access: %lld  < %lld", at, after);
            swear(ct < after, "create: %lld  < %lld", ct, after);
            swear(at < after, "update: %lld  < %lld", ut, after);
        }
    }
    files.closedir(&folder);
    r = files.rmdirs(tmp_dir);
    fatal_if(r != 0, "files.rmdirs(\"%s\") failed %s",
                     tmp_dir, str.error(r));
    r = files.unlink(tmp_file);
    fatal_if(r != 0, "files.unlink(\"%s\") failed %s",
                     tmp_file, str.error(r));
    fatal_if(files.chdir(cwd) != 0, "files.chdir(\"%s\") failed %s",
             cwd, str.error(runtime.err()));
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
}

#pragma pop_macro("verbose")

static void files_test_append_thread(void* p) {
    file_t* f = (file_t*)p;
    uint8_t data[256];
    for (int i = 0; i < 256; i++) { data[i] = (uint8_t)i; }
    int64_t transferred = 0;
    fatal_if(files.write(f, data, countof(data), &transferred) != 0 ||
             transferred != countof(data), "files.write()" files_test_failed);
}

static void files_test(void) {
    folders_test();
    uint64_t now = clock.microseconds(); // epoch time
    char tf[256]; // temporary file
    fatal_if(files.create_tmp(tf, countof(tf)) != 0,
            "files.create_tmp()" files_test_failed);
    uint8_t data[256];
    int64_t transferred = 0;
    for (int i = 0; i < 256; i++) { data[i] = (uint8_t)i; }
    {
        file_t* f = files.invalid;
        fatal_if(files.open(&f, tf,
                 files.o_wr | files.o_create | files.o_trunc) != 0 ||
                !files.is_valid(f), "files.open()" files_test_failed);
        fatal_if(files.write_fully(tf, data, countof(data), &transferred) != 0 ||
                 transferred != countof(data),
                "files.write_fully()" files_test_failed);
        fatal_if(files.open(&f, tf, files.o_rd) != 0 ||
                !files.is_valid(f), "files.open()" files_test_failed);
        for (int32_t i = 0; i < 256; i++) {
            for (int32_t j = 1; j < 256 - i; j++) {
                uint8_t test[countof(data)] = {0};
                int64_t position = i;
                fatal_if(files.seek(f, &position, files.seek_set) != 0 ||
                         position != i,
                        "files.seek(position: %lld) failed %s",
                         position, str.error(runtime.err()));
                fatal_if(files.read(f, test, j, &transferred) != 0 ||
                         transferred != j,
                        "files.read() transferred: %lld failed %s",
                        transferred, str.error(runtime.err()));
                for (int32_t k = 0; k < j; k++) {
                    swear(test[k] == data[i + k],
                         "Data mismatch at position: %d, length %d"
                         "test[%d]: 0x%02X != data[%d + %d]: 0x%02X ",
                          i, j,
                          k, test[k], i, k, data[i + k]);
                }
            }
        }
        swear((files.o_rd | files.o_wr) != files.o_rw);
        fatal_if(files.open(&f, tf, files.o_rw) != 0 || !files.is_valid(f),
                "files.open()" files_test_failed);
        for (int32_t i = 0; i < 256; i++) {
            uint8_t val = ~data[i];
            int64_t pos = i;
            fatal_if(files.seek(f, &pos, files.seek_set) != 0 || pos != i,
                    "files.seek() failed %s", runtime.err());
            fatal_if(files.write(f, &val, 1, &transferred) != 0 ||
                     transferred != 1, "files.write()" files_test_failed);
            pos = i;
            fatal_if(files.seek(f, &pos, files.seek_set) != 0 || pos != i,
                    "files.seek(pos: %lld i: %d) failed %s", pos, i, runtime.err());
            uint8_t read_val = 0;
            fatal_if(files.read(f, &read_val, 1, &transferred) != 0 ||
                     transferred != 1, "files.read()" files_test_failed);
            swear(read_val == val, "Data mismatch at position %d", i);
        }
        files_stat_t s = {0};
        files.stat(f, &s, false);
        uint64_t before = now - 1 * clock.usec_in_sec; // one second before now
        uint64_t after  = now + 2 * clock.usec_in_sec; // two seconds after
        swear(before <= s.created  && s.created  <= after,
             "before: %lld created: %lld after: %lld", before, s.created, after);
        swear(before <= s.accessed && s.accessed <= after,
             "before: %lld created: %lld accessed: %lld", before, s.accessed, after);
        swear(before <= s.updated  && s.updated  <= after,
             "before: %lld created: %lld updated: %lld", before, s.updated, after);
        files.close(f);
        fatal_if(files.open(&f, tf, files.o_wr | files.o_create | files.o_trunc) != 0 ||
                !files.is_valid(f), "files.open()" files_test_failed);
        files.stat(f, &s, false);
        swear(s.size == 0, "File is not empty after truncation. .size: %lld", s.size);
        files.close(f);
    }
    {  // Append test with threads
        file_t* f = files.invalid;
        fatal_if(files.open(&f, tf, files.o_rw | files.o_append) != 0 ||
                !files.is_valid(f), "files.open()" files_test_failed);
        thread_t thread1 = threads.start(files_test_append_thread, f);
        thread_t thread2 = threads.start(files_test_append_thread, f);
        threads.join(thread1, -1);
        threads.join(thread2, -1);
        files.close(f);
    }
    {   // write_fully, exists, is_folder, mkdirs, rmdirs, create_tmp, chmod777
        fatal_if(files.write_fully(tf, data, countof(data), &transferred) != 0 ||
                 transferred != countof(data),
                "files.write_fully() failed %s", runtime.err());
        fatal_if(!files.exists(tf), "file \"%s\" does not exist", tf);
        fatal_if(files.is_folder(tf), "%s is a folder", tf);
        fatal_if(files.chmod777(tf) != 0, "files.chmod777(\"%s\") failed %s",
                 tf, str.error(runtime.err()));
        char folder[256] = {0};
        strprintf(folder, "%s.folder\\subfolder", tf);
        fatal_if(files.mkdirs(folder) != 0, "files.mkdirs(\"%s\") failed %s",
            folder, str.error(runtime.err()));
        fatal_if(!files.is_folder(folder), "\"%s\" is not a folder", folder);
        fatal_if(files.chmod777(folder) != 0, "files.chmod777(\"%s\") failed %s",
                 folder, str.error(runtime.err()));
        fatal_if(files.rmdirs(folder) != 0, "files.rmdirs(\"%s\") failed %s",
                 folder, str.error(runtime.err()));
        fatal_if(files.exists(folder), "folder \"%s\" still exists", folder);
    }
    {   // getcwd, chdir
        const char* tmp = files.tmp();
        char cwd[256] = {0};
        fatal_if(files.cwd(cwd, sizeof(cwd)) != 0, "files.cwd() failed");
        fatal_if(files.chdir(tmp) != 0, "files.chdir(\"%s\") failed %s",
                 tmp, str.error(runtime.err()));
        // symlink
        if (processes.is_elevated()) {
            char sym_link[files_max_path];
            strprintf(sym_link, "%s.sym_link", tf);
            fatal_if(files.symlink(tf, sym_link) != 0,
                "files.symlink(\"%s\", \"%s\") failed %s",
                tf, sym_link, str.error(runtime.err()));
            fatal_if(!files.is_symlink(sym_link), "\"%s\" is not a sym_link", sym_link);
            fatal_if(files.unlink(sym_link) != 0, "files.unlink(\"%s\") failed %s",
                    sym_link, str.error(runtime.err()));
        } else {
            traceln("Skipping files.symlink test: process is not elevated");
        }
        // hard link
        char hard_link[files_max_path];
        strprintf(hard_link, "%s.hard_link", tf);
        fatal_if(files.link(tf, hard_link) != 0,
            "files.link(\"%s\", \"%s\") failed %s",
            tf, hard_link, str.error(runtime.err()));
        fatal_if(!files.exists(hard_link), "\"%s\" does not exist", hard_link);
        fatal_if(files.unlink(hard_link) != 0, "files.unlink(\"%s\") failed %s",
                 hard_link, str.error(runtime.err()));
        fatal_if(files.exists(hard_link), "\"%s\" still exists", hard_link);
        // copy, move:
        fatal_if(files.copy(tf, "copied_file") != 0,
            "files.copy(\"%s\", 'copied_file') failed %s",
            tf, str.error(runtime.err()));
        fatal_if(!files.exists("copied_file"), "'copied_file' does not exist");
        fatal_if(files.move("copied_file", "moved_file") != 0,
            "files.move('copied_file', 'moved_file') failed %s",
            str.error(runtime.err()));
        fatal_if(files.exists("copied_file"), "'copied_file' still exists");
        fatal_if(!files.exists("moved_file"), "'moved_file' does not exist");
        fatal_if(files.unlink("moved_file") != 0,
                "files.unlink('moved_file') failed %s",
                 str.error(runtime.err()));
        fatal_if(files.chdir(cwd) != 0, "files.chdir(\"%s\") failed %s",
                    cwd, str.error(runtime.err()));
    }
    fatal_if(files.unlink(tf) != 0);
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
}

#else

static void files_test(void) {}

#endif // RUNTIME_TESTS

#pragma pop_macro("files_test_failed")

files_if files = {
    .invalid  = (file_t*)INVALID_HANDLE_VALUE,
    // files_stat_t.type:
    .type_folder  = 0x00000010, // FILE_ATTRIBUTE_DIRECTORY
    .type_symlink = 0x00000400, // FILE_ATTRIBUTE_REPARSE_POINT
    .type_device  = 0x00000040, // FILE_ATTRIBUTE_DEVICE
    // seek() methods:
    .seek_set = SEEK_SET,
    .seek_cur = SEEK_CUR,
    .seek_end = SEEK_END,
    // open() flags: missing O_RSYNC, O_DSYNC, O_NONBLOCK, O_NOCTTY
    .o_rd     = O_RDONLY,
    .o_wr     = O_WRONLY,
    .o_rw     = O_RDWR,
    .o_append = O_APPEND,
    .o_create = O_CREAT,
    .o_excl   = O_EXCL,
    .o_trunc  = O_TRUNC,
    .o_sync   = O_SYNC,
    .open               = files_open,
    .is_valid           = files_is_valid,
    .seek               = files_seek,
    .stat               = files_stat,
    .read               = files_read,
    .write              = files_write,
    .flush              = files_flush,
    .close              = files_close,
    .write_fully        = files_write_fully,
    .exists             = files_exists,
    .is_folder          = files_is_folder,
    .is_symlink         = files_is_symlink,
    .mkdirs             = files_mkdirs,
    .rmdirs             = files_rmdirs,
    .create_tmp         = files_create_tmp,
    .chmod777           = files_chmod777,
    .unlink             = files_unlink,
    .link               = files_link,
    .symlink            = files_symlink,
    .basename           = files_basename,
    .copy               = files_copy,
    .move               = files_move,
    .cwd                = files_cwd,
    .chdir              = files_chdir,
    .tmp                = files_tmp,
    .bin                = files_bin,
    .opendir            = files_opendir,
    .readdir            = files_readdir,
    .closedir           = files_closedir,
    .test               = files_test
};
