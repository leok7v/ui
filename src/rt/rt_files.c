#include "rt/rt.h"
#include "rt/rt_win32.h"

// TODO: test FILE_APPEND_DATA
// https://learn.microsoft.com/en-us/windows/win32/fileio/appending-one-file-to-another-file?redirectedfrom=MSDN

// are posix and Win32 seek in agreement?
rt_static_assertion(SEEK_SET == FILE_BEGIN);
rt_static_assertion(SEEK_CUR == FILE_CURRENT);
rt_static_assertion(SEEK_END == FILE_END);

#ifndef O_SYNC
#define O_SYNC (0x10000)
#endif

static errno_t rt_files_open(rt_file_t* *file, const char* fn, int32_t f) {
    DWORD access = (f & rt_files.o_wr) ? GENERIC_WRITE :
                   (f & rt_files.o_rw) ? GENERIC_READ | GENERIC_WRITE :
                                      GENERIC_READ;
    access |= (f & rt_files.o_append) ? FILE_APPEND_DATA : 0;
    DWORD disposition =
        (f & rt_files.o_create) ? ((f & rt_files.o_excl)  ? CREATE_NEW :
                                (f & rt_files.o_trunc) ? CREATE_ALWAYS :
                                                      OPEN_ALWAYS) :
            (f & rt_files.o_trunc) ? TRUNCATE_EXISTING : OPEN_EXISTING;
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    DWORD attr = FILE_ATTRIBUTE_NORMAL;
    attr |= (f & O_SYNC) ? FILE_FLAG_WRITE_THROUGH : 0;
    *file = CreateFileA(fn, access, share, null, disposition, attr, null);
    return *file != INVALID_HANDLE_VALUE ? 0 : rt_core.err();
}

static bool rt_files_is_valid(rt_file_t* file) { // both null and rt_files.invalid
    return file != rt_files.invalid && file != null;
}

static errno_t rt_files_seek(rt_file_t* file, int64_t *position, int32_t method) {
    LARGE_INTEGER distance_to_move = { .QuadPart = *position };
    LARGE_INTEGER p = { 0 }; // pointer
    errno_t r = rt_b2e(SetFilePointerEx(file, distance_to_move, &p, (DWORD)method));
    if (r == 0) { *position = p.QuadPart; }
    return r;
}

static inline uint64_t rt_files_ft_to_us(FILETIME ft) { // us (microseconds)
    return (ft.dwLowDateTime | (((uint64_t)ft.dwHighDateTime) << 32)) / 10;
}

static int64_t rt_files_a2t(DWORD a) {
    int64_t type = 0;
    if (a & FILE_ATTRIBUTE_REPARSE_POINT) {
        type |= rt_files.type_symlink;
    }
    if (a & FILE_ATTRIBUTE_DIRECTORY) {
        type |= rt_files.type_folder;
    }
    if (a & FILE_ATTRIBUTE_DEVICE) {
        type |= rt_files.type_device;
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

static errno_t rt_files_stat(rt_file_t* file, rt_files_stat_t* s,
                             bool follow_symlink) {
    errno_t r = 0;
    BY_HANDLE_FILE_INFORMATION fi;
    rt_fatal_win32err(GetFileInformationByHandle(file, &fi));
    const bool symlink =
        (fi.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    if (follow_symlink && symlink) {
        const DWORD flags = FILE_NAME_NORMALIZED | VOLUME_NAME_DOS;
        DWORD n = GetFinalPathNameByHandleA(file, null, 0, flags);
        if (n == 0) {
            r = rt_core.err();
        } else {
            char* name = null;
            r = rt_heap.allocate(null, (void**)&name, (int64_t)n + 2, false);
            if (r == 0) {
                n = GetFinalPathNameByHandleA(file, name, n + 1, flags);
                if (n == 0) {
                    r = rt_core.err();
                } else {
                    rt_file_t* f = rt_files.invalid;
                    r = rt_files.open(&f, name, rt_files.o_rd);
                    if (r == 0) { // keep following:
                        r = rt_files.stat(f, s, follow_symlink);
                        rt_files.close(f);
                    }
                }
                rt_heap.deallocate(null, name);
            }
        }
    } else {
        s->size = (int64_t)((uint64_t)fi.nFileSizeLow |
                          (((uint64_t)fi.nFileSizeHigh) << 32));
        s->created  = rt_files_ft_to_us(fi.ftCreationTime); // since epoch
        s->accessed = rt_files_ft_to_us(fi.ftLastAccessTime);
        s->updated  = rt_files_ft_to_us(fi.ftLastWriteTime);
        s->type = rt_files_a2t(fi.dwFileAttributes);
    }
    return r;
}

static errno_t rt_files_read(rt_file_t* file, void* data, int64_t bytes, int64_t *transferred) {
    errno_t r = 0;
    *transferred = 0;
    while (bytes > 0 && r == 0) {
        DWORD chunk_size = (DWORD)(bytes > UINT32_MAX ? UINT32_MAX : bytes);
        DWORD bytes_read = 0;
        r = rt_b2e(ReadFile(file, data, chunk_size, &bytes_read, null));
        if (r == 0) {
            *transferred += bytes_read;
            bytes -= bytes_read;
            data = (uint8_t*)data + bytes_read;
        }
    }
    return r;
}

static errno_t rt_files_write(rt_file_t* file, const void* data, int64_t bytes, int64_t *transferred) {
    errno_t r = 0;
    *transferred = 0;
    while (bytes > 0 && r == 0) {
        DWORD chunk_size = (DWORD)(bytes > UINT32_MAX ? UINT32_MAX : bytes);
        DWORD bytes_read = 0;
        r = rt_b2e(WriteFile(file, data, chunk_size, &bytes_read, null));
        if (r == 0) {
            *transferred += bytes_read;
            bytes -= bytes_read;
            data = (const uint8_t*)data + bytes_read;
        }
    }
    return r;
}

static errno_t rt_files_flush(rt_file_t* file) {
    return rt_b2e(FlushFileBuffers(file));
}

static void rt_files_close(rt_file_t* file) {
    rt_win32_close_handle(file);
}

static errno_t rt_files_write_fully(const char* filename, const void* data,
                                 int64_t bytes, int64_t *transferred) {
    if (transferred != null) { *transferred = 0; }
    errno_t r = 0;
    const DWORD access = GENERIC_WRITE;
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const DWORD flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH;
    HANDLE file = CreateFileA(filename, access, share, null, CREATE_ALWAYS,
                              flags, null);
    if (file == INVALID_HANDLE_VALUE) {
        r = rt_core.err();
    } else {
        int64_t written = 0;
        const uint8_t* p = (const uint8_t*)data;
        while (r == 0 && bytes > 0) {
            uint64_t write = bytes >= UINT32_MAX ?
                (uint64_t)(UINT32_MAX) - 0xFFFFuLL : (uint64_t)bytes;
            rt_assert(0 < write && write < (uint64_t)UINT32_MAX);
            DWORD chunk = 0;
            r = rt_b2e(WriteFile(file, p, (DWORD)write, &chunk, null));
            written += chunk;
            bytes -= chunk;
        }
        if (transferred != null) { *transferred = written; }
        errno_t rc = rt_b2e(FlushFileBuffers(file));
        if (r == 0) { r = rc; }
        rt_win32_close_handle(file);
    }
    return r;
}

static errno_t rt_files_unlink(const char* pathname) {
    if (rt_files.is_folder(pathname)) {
        return rt_b2e(RemoveDirectoryA(pathname));
    } else {
        return rt_b2e(DeleteFileA(pathname));
    }
}

static errno_t rt_files_create_tmp(char* fn, int32_t count) {
    // create temporary file (not folder!) see folders_test() about racing
    rt_swear(fn != null && count > 0);
    const char* tmp = rt_files.tmp();
    errno_t r = 0;
    if (count < (int32_t)strlen(tmp) + 8) {
        r = ERROR_BUFFER_OVERFLOW;
    } else {
        rt_assert(count > (int32_t)strlen(tmp) + 8);
        // If GetTempFileNameA() succeeds, the return value is the length,
        // in chars, of the string copied to lpBuffer, not including the
        // terminating null character.If the function fails,
        // the return value is zero.
        if (count > (int32_t)strlen(tmp) + 8) {
            char prefix[4] = { 0 };
            r = GetTempFileNameA(tmp, prefix, 0, fn) == 0 ? rt_core.err() : 0;
            if (r == 0) {
                rt_assert(rt_files.exists(fn) && !rt_files.is_folder(fn));
            } else {
                rt_println("GetTempFileNameA() failed %s", rt_strerr(r));
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

#define rt_files_acl_args(acl) DACL_SECURITY_INFORMATION, null, null, acl, null

#define rt_files_get_acl(obj, type, acl, sd) (errno_t)(         \
    (type == SE_FILE_OBJECT ? GetNamedSecurityInfoA((char*)obj, \
             SE_FILE_OBJECT, rt_files_acl_args(acl), &sd) :     \
    (type == SE_KERNEL_OBJECT) ? GetSecurityInfo((HANDLE)obj,   \
             SE_KERNEL_OBJECT, rt_files_acl_args(acl), &sd) :   \
    ERROR_INVALID_PARAMETER))

#define rt_files_set_acl(obj, type, acl) (errno_t)(             \
    (type == SE_FILE_OBJECT ? SetNamedSecurityInfoA((char*)obj, \
             SE_FILE_OBJECT, rt_files_acl_args(acl)) :          \
    (type == SE_KERNEL_OBJECT) ? SetSecurityInfo((HANDLE)obj,   \
             SE_KERNEL_OBJECT, rt_files_acl_args(acl)) :        \
    ERROR_INVALID_PARAMETER))

static errno_t rt_files_acl_add_ace(ACL* acl, SID* sid, uint32_t mask,
                                 ACL** free_me, byte flags) {
    ACL_SIZE_INFORMATION info = {0};
    ACL* bigger = null;
    uint32_t bytes_needed = sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(sid)
                          - sizeof(DWORD);
    errno_t r = rt_b2e(GetAclInformation(acl, &info, sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation));
    if (r == 0 && info.AclBytesFree < bytes_needed) {
        const int64_t bytes = (int64_t)(info.AclBytesInUse + bytes_needed);
        r = rt_heap.allocate(null, (void**)&bigger, bytes, true);
        if (r == 0) {
            r = rt_b2e(InitializeAcl((ACL*)bigger,
                    info.AclBytesInUse + bytes_needed, ACL_REVISION));
        }
    }
    if (r == 0 && bigger != null) {
        for (int32_t i = 0; i < (int32_t)info.AceCount; i++) {
            ACCESS_ALLOWED_ACE* ace = null;
            r = rt_b2e(GetAce(acl, (DWORD)i, (void**)&ace));
            if (r != 0) { break; }
            r = rt_b2e(AddAce(bigger, ACL_REVISION, MAXDWORD, ace,
                           ace->Header.AceSize));
            if (r != 0) { break; }
        }
    }
    if (r == 0) {
        ACCESS_ALLOWED_ACE* ace = null;
        r = rt_heap.allocate(null, (void**)&ace, bytes_needed, true);
        if (r == 0) {
            ace->Header.AceFlags = flags;
            ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
            ace->Header.AceSize = (WORD)bytes_needed;
            ace->Mask = mask;
            ace->SidStart = sizeof(ACCESS_ALLOWED_ACE);
            memcpy(&ace->SidStart, sid, GetLengthSid(sid));
            r = rt_b2e(AddAce(bigger != null ? bigger : acl, ACL_REVISION, MAXDWORD,
                           ace, bytes_needed));
            rt_heap.deallocate(null, ace);
        }
    }
    *free_me = bigger;
    return r;
}

static errno_t rt_files_lookup_sid(ACCESS_ALLOWED_ACE* ace) {
    // handy for debugging
    SID* sid = (SID*)&ace->SidStart;
    DWORD l1 = 128, l2 = 128;
    char account[128];
    char group[128];
    SID_NAME_USE use;
    errno_t r = rt_b2e(LookupAccountSidA(null, sid, account,
                                     &l1, group, &l2, &use));
    if (r == 0) {
        rt_println("%s/%s: type: %d, mask: 0x%X, flags:%d",
                group, account,
                ace->Header.AceType, ace->Mask, ace->Header.AceFlags);
    } else {
        rt_println("LookupAccountSidA() failed %s", rt_strerr(r));
    }
    return r;
}

static errno_t rt_files_add_acl_ace(void* obj, int32_t obj_type,
                                 int32_t sid_type, uint32_t mask) {
    uint8_t stack[SECURITY_MAX_SID_SIZE] = {0};
    DWORD n = rt_countof(stack);
    SID* sid = (SID*)stack;
    errno_t r = rt_b2e(CreateWellKnownSid((WELL_KNOWN_SID_TYPE)sid_type,
                                       null, sid, &n));
    if (r != 0) {
        return ERROR_INVALID_PARAMETER;
    }
    ACL* acl = null;
    void* sd = null;
    r = rt_files_get_acl(obj, obj_type, &acl, sd);
    if (r == 0) {
        ACCESS_ALLOWED_ACE* found = null;
        for (int32_t i = 0; i < acl->AceCount; i++) {
            ACCESS_ALLOWED_ACE* ace = null;
            r = rt_b2e(GetAce(acl, (DWORD)i, (void**)&ace));
            if (r != 0) { break; }
            if (EqualSid((SID*)&ace->SidStart, sid)) {
                if (ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE &&
                   (ace->Header.AceFlags & INHERITED_ACE) == 0) {
                    found = ace;
                } else if (ace->Header.AceType !=
                           ACCESS_ALLOWED_ACE_TYPE) {
                    rt_println("%d ACE_TYPE is not supported.",
                             ace->Header.AceType);
                    r = ERROR_INVALID_PARAMETER;
                }
                break;
            }
        }
        if (r == 0 && found) {
            if ((found->Mask & mask) != mask) {
//              rt_println("updating existing ace");
                found->Mask |= mask;
                r = rt_files_set_acl(obj, obj_type, acl);
            } else {
//              rt_println("desired access is already allowed by ace");
            }
        } else if (r == 0) {
//          rt_println("inserting new ace");
            ACL* new_acl = null;
            byte flags = obj_type == SE_FILE_OBJECT ?
                CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE : 0;
            r = rt_files_acl_add_ace(acl, sid, mask, &new_acl, flags);
            if (r == 0) {
                r = rt_files_set_acl(obj, obj_type, (new_acl != null ? new_acl : acl));
            }
            if (new_acl != null) { rt_heap.deallocate(null, new_acl); }
        }
    }
    if (sd != null) { LocalFree(sd); }
    return r;
}

#pragma pop_macro("files_set_acl")
#pragma pop_macro("files_get_acl")
#pragma pop_macro("files_acl_args")

static errno_t rt_files_chmod777(const char* pathname) {
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID everyone = null; // Create a well-known SID for the Everyone group.
    rt_fatal_win32err(AllocateAndInitializeSid(&SIDAuthWorld, 1,
             SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &everyone));
    EXPLICIT_ACCESSA ea[1] = { { 0 } };
    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    ea[0].grfAccessPermissions = 0xFFFFFFFF;
    ea[0].grfAccessMode  = GRANT_ACCESS; // The ACE will allow everyone all access.
    ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPSTR)everyone;
    // Create a new ACL that contains the new ACEs.
    ACL* acl = null;
    rt_fatal_if_error(SetEntriesInAclA(1, ea, null, &acl));
    // Initialize a security descriptor.
    uint8_t stack[SECURITY_DESCRIPTOR_MIN_LENGTH] = {0};
    SECURITY_DESCRIPTOR* sd = (SECURITY_DESCRIPTOR*)stack;
    rt_fatal_win32err(InitializeSecurityDescriptor(sd,
        SECURITY_DESCRIPTOR_REVISION));
    // Add the ACL to the security descriptor.
    rt_fatal_win32err(SetSecurityDescriptorDacl(sd,
        /* present flag: */ true, acl, /* not a default DACL: */  false));
    // Change the security attributes
    errno_t r = rt_b2e(SetFileSecurityA(pathname, DACL_SECURITY_INFORMATION, sd));
    if (r != 0) {
        rt_println("chmod777(%s) failed %s", pathname, rt_strerr(r));
    }
    if (everyone != null) { FreeSid(everyone); }
    if (acl != null) { LocalFree(acl); }
    return r;
}

// https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createdirectorya
// "If lpSecurityAttributes is null, the directory gets a default security
//  descriptor. The ACLs in the default security descriptor for a directory
//  are inherited from its parent directory."

static errno_t rt_files_mkdirs(const char* dir) {
    const int32_t n = (int32_t)strlen(dir) + 1;
    char* s = null;
    errno_t r = rt_heap.allocate(null, (void**)&s, n, true);
    const char* next = strchr(dir, '\\');
    if (next == null) { next = strchr(dir, '/'); }
    while (r == 0 && next != null) {
        if (next > dir && *(next - 1) != ':') {
            memcpy(s, dir, (size_t)(next - dir));
            r = rt_b2e(CreateDirectoryA(s, null));
            if (r == ERROR_ALREADY_EXISTS) { r = 0; }
        }
        if (r == 0) {
            const char* prev = ++next;
            next = strchr(prev, '\\');
            if (next == null) { next = strchr(prev, '/'); }
        }
    }
    if (r == 0) {
        r = rt_b2e(CreateDirectoryA(dir, null));
    }
    rt_heap.deallocate(null, s);
    return r == ERROR_ALREADY_EXISTS ? 0 : r;
}

#pragma push_macro("rt_files_realloc_path")
#pragma push_macro("rt_files_append_name")

#define rt_files_realloc_path(r, pn, pnc, fn, name) do {                \
    const int32_t bytes = (int32_t)(strlen(fn) + strlen(name) + 3);     \
    if (bytes > pnc) {                                                  \
        r = rt_heap.reallocate(null, (void**)&pn, bytes, false);        \
        if (r != 0) {                                                   \
            pnc = bytes;                                                \
        } else {                                                        \
            rt_heap.deallocate(null, pn);                               \
            pn = null;                                                  \
        }                                                               \
    }                                                                   \
} while (0)

#define rt_files_append_name(pn, pnc, fn, name) do {     \
    if (strcmp(fn, "\\") == 0 || strcmp(fn, "/") == 0) { \
        rt_str.format(pn, pnc, "\\%s", name);            \
    } else {                                             \
        rt_str.format(pn, pnc, "%.*s\\%s", k, fn, name); \
    }                                                    \
} while (0)

static errno_t rt_files_rmdirs(const char* fn) {
    rt_files_stat_t st;
    rt_folder_t folder;
    errno_t r = rt_files.opendir(&folder, fn);
    if (r == 0) {
        int32_t k = (int32_t)strlen(fn);
        // remove trailing backslash (except if it is root: "/" or "\\")
        if (k > 1 && (fn[k - 1] == '/' || fn[k - 1] == '\\')) {
            k--;
        }
        int32_t pnc = 64 * 1024; // pathname "pn" capacity in bytes
        char* pn = null;
        r = rt_heap.allocate(null, (void**)&pn, pnc, false);
        while (r == 0) {
            // recurse into sub folders and remove them first
            // do NOT follow symlinks - it could be disastrous
            const char* name = rt_files.readdir(&folder, &st);
            if (name == null) { break; }
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0 &&
                (st.type & rt_files.type_symlink) == 0 &&
                (st.type & rt_files.type_folder) != 0) {
                rt_files_realloc_path(r, pn, pnc, fn, name);
                if (r == 0) {
                    rt_files_append_name(pn, pnc, fn, name);
                    r = rt_files.rmdirs(pn);
                }
            }
        }
        rt_files.closedir(&folder);
        r = rt_files.opendir(&folder, fn);
        while (r == 0) {
            const char* name = rt_files.readdir(&folder, &st);
            if (name == null) { break; }
            // symlinks are already removed as normal files
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0 &&
                (st.type & rt_files.type_folder) == 0) {
                rt_files_realloc_path(r, pn, pnc, fn, name);
                if (r == 0) {
                    rt_files_append_name(pn, pnc, fn, name);
                    r = rt_files.unlink(pn);
                    if (r != 0) {
                        rt_println("remove(%s) failed %s", pn, rt_strerr(r));
                    }
                }
            }
        }
        rt_heap.deallocate(null, pn);
        rt_files.closedir(&folder);
    }
    if (r == 0) { r = rt_files.unlink(fn); }
    return r;
}

#pragma pop_macro("rt_files_append_name")
#pragma pop_macro("rt_files_realloc_path")

static bool rt_files_exists(const char* path) {
    return PathFileExistsA(path);
}

static bool rt_files_is_folder(const char* path) {
    return PathIsDirectoryA(path);
}

static bool rt_files_is_symlink(const char* filename) {
    DWORD attributes = GetFileAttributesA(filename);
    return attributes != INVALID_FILE_ATTRIBUTES &&
          (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

static const char* rt_files_basename(const char* pathname) {
    const char* bn = strrchr(pathname, '\\');
    if (bn == null) { bn = strrchr(pathname, '/'); }
    return bn != null ? bn + 1 : pathname;
}

static errno_t rt_files_copy(const char* s, const char* d) {
    return rt_b2e(CopyFileA(s, d, false));
}

static errno_t rt_files_move(const char* s, const char* d) {
    static const DWORD flags =
        MOVEFILE_REPLACE_EXISTING |
        MOVEFILE_COPY_ALLOWED |
        MOVEFILE_WRITE_THROUGH;
    return rt_b2e(MoveFileExA(s, d, flags));
}

static errno_t rt_files_link(const char* from, const char* to) {
    // note reverse order of parameters:
    return rt_b2e(CreateHardLinkA(to, from, null));
}

static errno_t rt_files_symlink(const char* from, const char* to) {
    // The correct order of parameters for CreateSymbolicLinkA is:
    // CreateSymbolicLinkA(symlink_to_create, existing_file, flags);
    DWORD flags = rt_files.is_folder(from) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
    return rt_b2e(CreateSymbolicLinkA(to, from, flags));
}

static const char* rt_files_known_folder(int32_t kf) {
    // known folder ids order must match enum see:
    static const GUID* kf_ids[] = {
        &FOLDERID_Profile,
        &FOLDERID_Desktop,
        &FOLDERID_Documents,
        &FOLDERID_Downloads,
        &FOLDERID_Music,
        &FOLDERID_Pictures,
        &FOLDERID_Videos,
        &FOLDERID_Public,
        &FOLDERID_ProgramFiles,
        &FOLDERID_ProgramData
    };
    static rt_file_name_t known_folders[rt_countof(kf_ids)];
    rt_fatal_if(!(0 <= kf && kf < rt_countof(kf_ids)), "invalid kf=%d", kf);
    if (known_folders[kf].s[0] == 0) {
        uint16_t* path = null;
        rt_fatal_if_error(SHGetKnownFolderPath(kf_ids[kf], 0, null, &path));
        const int32_t n = rt_countof(known_folders[kf].s);
        rt_str.utf16to8(known_folders[kf].s, n, path, -1);
        CoTaskMemFree(path);
	}
    return known_folders[kf].s;
}

static const char* rt_files_bin(void) {
    return rt_files_known_folder(rt_files.folder.bin);
}

static const char* rt_files_data(void) {
    return rt_files_known_folder(rt_files.folder.data);
}

static const char* rt_files_tmp(void) {
    static char tmp[rt_files_max_path];
    if (tmp[0] == 0) {
        // If GetTempPathA() succeeds, the return value is the length,
        // in chars, of the string copied to lpBuffer, not including
        // the terminating null character. If the function fails, the
        // return value is zero.
        errno_t r = GetTempPathA(rt_countof(tmp), tmp) == 0 ? rt_core.err() : 0;
        rt_fatal_if(r != 0, "GetTempPathA() failed %s", rt_strerr(r));
    }
    return tmp;
}

static errno_t rt_files_cwd(char* fn, int32_t count) {
    rt_swear(count > 1);
    DWORD bytes = (DWORD)(count - 1);
    errno_t r = rt_b2e(GetCurrentDirectoryA(bytes, fn));
    fn[count - 1] = 0; // always
    return r;
}

static errno_t rt_files_chdir(const char* fn) {
    return rt_b2e(SetCurrentDirectoryA(fn));
}

typedef struct rt_files_dir_s {
    HANDLE handle;
    WIN32_FIND_DATAA find; // On Win64: 320 bytes
} rt_files_dir_t;

rt_static_assertion(sizeof(rt_files_dir_t) <= sizeof(rt_folder_t));

static errno_t rt_files_opendir(rt_folder_t* folder, const char* folder_name) {
    rt_files_dir_t* d = (rt_files_dir_t*)(void*)folder;
    int32_t n = (int32_t)strlen(folder_name);
    char* fn = null;
    // extra room for "\*" suffix
    errno_t r = rt_heap.allocate(null, (void**)&fn, (int64_t)n + 3, false);
    if (r == 0) {
        rt_str.format(fn, n + 3, "%s\\*", folder_name);
        fn[n + 2] = 0;
        d->handle = FindFirstFileA(fn, &d->find);
        if (d->handle == INVALID_HANDLE_VALUE) { r = rt_core.err(); }
        rt_heap.deallocate(null, fn);
    }
    return r;
}

static uint64_t rt_files_ft2us(FILETIME* ft) { // 100ns units to microseconds:
    return (((uint64_t)ft->dwHighDateTime) << 32 | ft->dwLowDateTime) / 10;
}

static const char* rt_files_readdir(rt_folder_t* folder, rt_files_stat_t* s) {
    const char* fn = null;
    rt_files_dir_t* d = (rt_files_dir_t*)(void*)folder;
    if (FindNextFileA(d->handle, &d->find)) {
        fn = d->find.cFileName;
        // Ensure zero termination
        d->find.cFileName[rt_countof(d->find.cFileName) - 1] = 0x00;
        if (s != null) {
            s->accessed = rt_files_ft2us(&d->find.ftLastAccessTime);
            s->created = rt_files_ft2us(&d->find.ftCreationTime);
            s->updated = rt_files_ft2us(&d->find.ftLastWriteTime);
            s->type = rt_files_a2t(d->find.dwFileAttributes);
            s->size = (int64_t)((((uint64_t)d->find.nFileSizeHigh) << 32) |
                                  (uint64_t)d->find.nFileSizeLow);
        }
    }
    return fn;
}

static void rt_files_closedir(rt_folder_t* folder) {
    rt_files_dir_t* d = (rt_files_dir_t*)(void*)folder;
    rt_fatal_win32err(FindClose(d->handle));
}

#pragma push_macro("files_test_failed")

#ifdef RT_TESTS

// TODO: change rt_fatal_if() to swear()

#define rt_files_test_failed " failed %s", rt_strerr(rt_core.err())

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                       \
    if (rt_debug.verbosity.level >= rt_debug.verbosity.trace) { \
        rt_println(__VA_ARGS__);                                   \
    }                                                           \
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
    rt_clock.local(us, &year, &month, &day, &hh, &mm, &ss, &ms, &mc);
    rt_println("%-7s: %04d-%02d-%02d %02d:%02d:%02d.%03d:%03d",
            label, year, month, day, hh, mm, ss, ms, mc);
}

static void folders_test(void) {
    uint64_t now = rt_clock.microseconds(); // microseconds since epoch
    uint64_t before = now - 1 * (uint64_t)rt_clock.usec_in_sec; // one second earlier
    uint64_t after  = now + 2 * (uint64_t)rt_clock.usec_in_sec; // two seconds later
    int32_t year = 0;
    int32_t month = 0;
    int32_t day = 0;
    int32_t hh = 0;
    int32_t mm = 0;
    int32_t ss = 0;
    int32_t ms = 0;
    int32_t mc = 0;
    rt_clock.local(now, &year, &month, &day, &hh, &mm, &ss, &ms, &mc);
    verbose("now: %04d-%02d-%02d %02d:%02d:%02d.%03d:%03d",
             year, month, day, hh, mm, ss, ms, mc);
    // Test cwd, setcwd
    const char* tmp = rt_files.tmp();
    char cwd[256] = { 0 };
    rt_fatal_if(rt_files.cwd(cwd, sizeof(cwd)) != 0, "rt_files.cwd() failed");
    rt_fatal_if(rt_files.chdir(tmp) != 0, "rt_files.chdir(\"%s\") failed %s",
                tmp, rt_strerr(rt_core.err()));
    // there is no racing free way to create temporary folder
    // without having a temporary file for the duration of folder usage:
    char tmp_file[rt_files_max_path]; // create_tmp() is thread safe race free:
    errno_t r = rt_files.create_tmp(tmp_file, rt_countof(tmp_file));
    rt_fatal_if(r != 0, "rt_files.create_tmp() failed %s", rt_strerr(r));
    char tmp_dir[rt_files_max_path];
    rt_str_printf(tmp_dir, "%s.dir", tmp_file);
    r = rt_files.mkdirs(tmp_dir);
    rt_fatal_if(r != 0, "rt_files.mkdirs(%s) failed %s", tmp_dir, rt_strerr(r));
    verbose("%s", tmp_dir);
    rt_folder_t folder;
    char pn[rt_files_max_path] = { 0 };
    rt_str_printf(pn, "%s/file", tmp_dir);
    // cannot test symlinks because they are only
    // available to Administrators and in Developer mode
//  char sym[rt_files_max_path] = { 0 };
    char hard[rt_files_max_path] = { 0 };
    char sub[rt_files_max_path] = { 0 };
    rt_str_printf(hard, "%s/hard", tmp_dir);
    rt_str_printf(sub, "%s/subd", tmp_dir);
    const char* content = "content";
    int64_t transferred = 0;
    r = rt_files.write_fully(pn, content, (int64_t)strlen(content), &transferred);
    rt_fatal_if(r != 0, "rt_files.write_fully(\"%s\") failed %s", pn, rt_strerr(r));
    rt_swear(transferred == (int64_t)strlen(content));
    r = rt_files.link(pn, hard);
    rt_fatal_if(r != 0, "rt_files.link(\"%s\", \"%s\") failed %s",
                      pn, hard, rt_strerr(r));
    r = rt_files.mkdirs(sub);
    rt_fatal_if(r != 0, "rt_files.mkdirs(\"%s\") failed %s", sub, rt_strerr(r));
    r = rt_files.opendir(&folder, tmp_dir);
    rt_fatal_if(r != 0, "rt_files.opendir(\"%s\") failed %s", tmp_dir, rt_strerr(r));
    for (;;) {
        rt_files_stat_t st = { 0 };
        const char* name = rt_files.readdir(&folder, &st);
        if (name == null) { break; }
        uint64_t at = st.accessed;
        uint64_t ct = st.created;
        uint64_t ut = st.updated;
        rt_swear(ct <= at && ct <= ut);
        rt_clock.local(ct, &year, &month, &day, &hh, &mm, &ss, &ms, &mc);
        bool is_folder = st.type & rt_files.type_folder;
        bool is_symlink = st.type & rt_files.type_symlink;
        int64_t bytes = st.size;
        verbose("%s: %04d-%02d-%02d %02d:%02d:%02d.%03d:%03d %lld bytes %s%s",
                name, year, month, day, hh, mm, ss, ms, mc,
                bytes, is_folder ? "[folder]" : "", is_symlink ? "[symlink]" : "");
        if (strcmp(name, "file") == 0 || strcmp(name, "hard") == 0) {
            rt_swear(bytes == (int64_t)strlen(content),
                    "size of \"%s\": %lld is incorrect expected: %d",
                    name, bytes, transferred);
        }
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            rt_swear(is_folder, "\"%s\" is_folder: %d", name, is_folder);
        } else {
            rt_swear((strcmp(name, "subd") == 0) == is_folder,
                  "\"%s\" is_folder: %d", name, is_folder);
            // empirically timestamps are imprecise on NTFS
            rt_swear(at >= before, "access: %lld  >= %lld", at, before);
            if (ct < before || ut < before || at >= after || ct >= after || ut >= after) {
                rt_println("file: %s", name);
                folders_dump_time("before", before);
                folders_dump_time("create", ct);
                folders_dump_time("update", ut);
                folders_dump_time("access", at);
            }
            rt_swear(ct >= before, "create: %lld  >= %lld", ct, before);
            rt_swear(ut >= before, "update: %lld  >= %lld", ut, before);
            // and no later than 2 seconds since folders_test()
            rt_swear(at < after, "access: %lld  < %lld", at, after);
            rt_swear(ct < after, "create: %lld  < %lld", ct, after);
            rt_swear(at < after, "update: %lld  < %lld", ut, after);
        }
    }
    rt_files.closedir(&folder);
    r = rt_files.rmdirs(tmp_dir);
    rt_fatal_if(r != 0, "rt_files.rmdirs(\"%s\") failed %s",
                     tmp_dir, rt_strerr(r));
    r = rt_files.unlink(tmp_file);
    rt_fatal_if(r != 0, "rt_files.unlink(\"%s\") failed %s",
                     tmp_file, rt_strerr(r));
    rt_fatal_if(rt_files.chdir(cwd) != 0, "rt_files.chdir(\"%s\") failed %s",
             cwd, rt_strerr(rt_core.err()));
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

#pragma pop_macro("verbose")

static void rt_files_test_append_thread(void* p) {
    rt_file_t* f = (rt_file_t*)p;
    uint8_t data[256] = {0};
    for (int i = 0; i < 256; i++) { data[i] = (uint8_t)i; }
    int64_t transferred = 0;
    rt_fatal_if(rt_files.write(f, data, rt_countof(data), &transferred) != 0 ||
             transferred != rt_countof(data), "rt_files.write()" rt_files_test_failed);
}

static void rt_files_test(void) {
    folders_test();
    uint64_t now = rt_clock.microseconds(); // epoch time
    char tf[256]; // temporary file
    rt_fatal_if(rt_files.create_tmp(tf, rt_countof(tf)) != 0,
            "rt_files.create_tmp()" rt_files_test_failed);
    uint8_t data[256] = {0};
    int64_t transferred = 0;
    for (int i = 0; i < 256; i++) { data[i] = (uint8_t)i; }
    {
        rt_file_t* f = rt_files.invalid;
        rt_fatal_if(rt_files.open(&f, tf,
                 rt_files.o_wr | rt_files.o_create | rt_files.o_trunc) != 0 ||
                !rt_files.is_valid(f), "rt_files.open()" rt_files_test_failed);
        rt_fatal_if(rt_files.write_fully(tf, data, rt_countof(data), &transferred) != 0 ||
                 transferred != rt_countof(data),
                "rt_files.write_fully()" rt_files_test_failed);
        rt_fatal_if(rt_files.open(&f, tf, rt_files.o_rd) != 0 ||
                !rt_files.is_valid(f), "rt_files.open()" rt_files_test_failed);
        for (int32_t i = 0; i < 256; i++) {
            for (int32_t j = 1; j < 256 - i; j++) {
                uint8_t test[rt_countof(data)] = { 0 };
                int64_t position = i;
                rt_fatal_if(rt_files.seek(f, &position, rt_files.seek_set) != 0 ||
                         position != i,
                        "rt_files.seek(position: %lld) failed %s",
                         position, rt_strerr(rt_core.err()));
                rt_fatal_if(rt_files.read(f, test, j, &transferred) != 0 ||
                         transferred != j,
                        "rt_files.read() transferred: %lld failed %s",
                        transferred, rt_strerr(rt_core.err()));
                for (int32_t k = 0; k < j; k++) {
                    rt_swear(test[k] == data[i + k],
                         "Data mismatch at position: %d, length %d"
                         "test[%d]: 0x%02X != data[%d + %d]: 0x%02X ",
                          i, j,
                          k, test[k], i, k, data[i + k]);
                }
            }
        }
        rt_swear((rt_files.o_rd | rt_files.o_wr) != rt_files.o_rw);
        rt_fatal_if(rt_files.open(&f, tf, rt_files.o_rw) != 0 || !rt_files.is_valid(f),
                "rt_files.open()" rt_files_test_failed);
        for (int32_t i = 0; i < 256; i++) {
            uint8_t val = ~data[i];
            int64_t pos = i;
            rt_fatal_if(rt_files.seek(f, &pos, rt_files.seek_set) != 0 || pos != i,
                    "rt_files.seek() failed %s", rt_core.err());
            rt_fatal_if(rt_files.write(f, &val, 1, &transferred) != 0 ||
                     transferred != 1, "rt_files.write()" rt_files_test_failed);
            pos = i;
            rt_fatal_if(rt_files.seek(f, &pos, rt_files.seek_set) != 0 || pos != i,
                    "rt_files.seek(pos: %lld i: %d) failed %s", pos, i, rt_core.err());
            uint8_t read_val = 0;
            rt_fatal_if(rt_files.read(f, &read_val, 1, &transferred) != 0 ||
                     transferred != 1, "rt_files.read()" rt_files_test_failed);
            rt_swear(read_val == val, "Data mismatch at position %d", i);
        }
        rt_files_stat_t s = { 0 };
        rt_files.stat(f, &s, false);
        uint64_t before = now - 1 * (uint64_t)rt_clock.usec_in_sec; // one second before now
        uint64_t after  = now + 2 * (uint64_t)rt_clock.usec_in_sec; // two seconds after
        rt_swear(before <= s.created  && s.created  <= after,
             "before: %lld created: %lld after: %lld", before, s.created, after);
        rt_swear(before <= s.accessed && s.accessed <= after,
             "before: %lld created: %lld accessed: %lld", before, s.accessed, after);
        rt_swear(before <= s.updated  && s.updated  <= after,
             "before: %lld created: %lld updated: %lld", before, s.updated, after);
        rt_files.close(f);
        rt_fatal_if(rt_files.open(&f, tf, rt_files.o_wr | rt_files.o_create | rt_files.o_trunc) != 0 ||
                !rt_files.is_valid(f), "rt_files.open()" rt_files_test_failed);
        rt_files.stat(f, &s, false);
        rt_swear(s.size == 0, "File is not empty after truncation. .size: %lld", s.size);
        rt_files.close(f);
    }
    {  // Append test with threads
        rt_file_t* f = rt_files.invalid;
        rt_fatal_if(rt_files.open(&f, tf, rt_files.o_rw | rt_files.o_append) != 0 ||
                !rt_files.is_valid(f), "rt_files.open()" rt_files_test_failed);
        rt_thread_t thread1 = rt_thread.start(rt_files_test_append_thread, f);
        rt_thread_t thread2 = rt_thread.start(rt_files_test_append_thread, f);
        rt_thread.join(thread1, -1);
        rt_thread.join(thread2, -1);
        rt_files.close(f);
    }
    {   // write_fully, exists, is_folder, mkdirs, rmdirs, create_tmp, chmod777
        rt_fatal_if(rt_files.write_fully(tf, data, rt_countof(data), &transferred) != 0 ||
                 transferred != rt_countof(data),
                "rt_files.write_fully() failed %s", rt_core.err());
        rt_fatal_if(!rt_files.exists(tf), "file \"%s\" does not exist", tf);
        rt_fatal_if(rt_files.is_folder(tf), "%s is a folder", tf);
        rt_fatal_if(rt_files.chmod777(tf) != 0, "rt_files.chmod777(\"%s\") failed %s",
                 tf, rt_strerr(rt_core.err()));
        char folder[256] = { 0 };
        rt_str_printf(folder, "%s.folder\\subfolder", tf);
        rt_fatal_if(rt_files.mkdirs(folder) != 0, "rt_files.mkdirs(\"%s\") failed %s",
            folder, rt_strerr(rt_core.err()));
        rt_fatal_if(!rt_files.is_folder(folder), "\"%s\" is not a folder", folder);
        rt_fatal_if(rt_files.chmod777(folder) != 0, "rt_files.chmod777(\"%s\") failed %s",
                 folder, rt_strerr(rt_core.err()));
        rt_fatal_if(rt_files.rmdirs(folder) != 0, "rt_files.rmdirs(\"%s\") failed %s",
                 folder, rt_strerr(rt_core.err()));
        rt_fatal_if(rt_files.exists(folder), "folder \"%s\" still exists", folder);
    }
    {   // getcwd, chdir
        const char* tmp = rt_files.tmp();
        char cwd[256] = { 0 };
        rt_fatal_if(rt_files.cwd(cwd, sizeof(cwd)) != 0, "rt_files.cwd() failed");
        rt_fatal_if(rt_files.chdir(tmp) != 0, "rt_files.chdir(\"%s\") failed %s",
                 tmp, rt_strerr(rt_core.err()));
        // symlink
        if (rt_processes.is_elevated()) {
            char sym_link[rt_files_max_path];
            rt_str_printf(sym_link, "%s.sym_link", tf);
            rt_fatal_if(rt_files.symlink(tf, sym_link) != 0,
                "rt_files.symlink(\"%s\", \"%s\") failed %s",
                tf, sym_link, rt_strerr(rt_core.err()));
            rt_fatal_if(!rt_files.is_symlink(sym_link), "\"%s\" is not a sym_link", sym_link);
            rt_fatal_if(rt_files.unlink(sym_link) != 0, "rt_files.unlink(\"%s\") failed %s",
                    sym_link, rt_strerr(rt_core.err()));
        } else {
            rt_println("Skipping rt_files.symlink test: process is not elevated");
        }
        // hard link
        char hard_link[rt_files_max_path];
        rt_str_printf(hard_link, "%s.hard_link", tf);
        rt_fatal_if(rt_files.link(tf, hard_link) != 0,
            "rt_files.link(\"%s\", \"%s\") failed %s",
            tf, hard_link, rt_strerr(rt_core.err()));
        rt_fatal_if(!rt_files.exists(hard_link), "\"%s\" does not exist", hard_link);
        rt_fatal_if(rt_files.unlink(hard_link) != 0, "rt_files.unlink(\"%s\") failed %s",
                 hard_link, rt_strerr(rt_core.err()));
        rt_fatal_if(rt_files.exists(hard_link), "\"%s\" still exists", hard_link);
        // copy, move:
        rt_fatal_if(rt_files.copy(tf, "copied_file") != 0,
            "rt_files.copy(\"%s\", 'copied_file') failed %s",
            tf, rt_strerr(rt_core.err()));
        rt_fatal_if(!rt_files.exists("copied_file"), "'copied_file' does not exist");
        rt_fatal_if(rt_files.move("copied_file", "moved_file") != 0,
            "rt_files.move('copied_file', 'moved_file') failed %s",
            rt_strerr(rt_core.err()));
        rt_fatal_if(rt_files.exists("copied_file"), "'copied_file' still exists");
        rt_fatal_if(!rt_files.exists("moved_file"), "'moved_file' does not exist");
        rt_fatal_if(rt_files.unlink("moved_file") != 0,
                "rt_files.unlink('moved_file') failed %s",
                 rt_strerr(rt_core.err()));
        rt_fatal_if(rt_files.chdir(cwd) != 0, "rt_files.chdir(\"%s\") failed %s",
                    cwd, rt_strerr(rt_core.err()));
    }
    rt_fatal_if(rt_files.unlink(tf) != 0);
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

#else

static void rt_files_test(void) {}

#endif // RT_TESTS

#pragma pop_macro("files_test_failed")

rt_files_if rt_files = {
    .invalid  = (rt_file_t*)INVALID_HANDLE_VALUE,
    // rt_files_stat_t.type:
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
    // known folders ids:
    .folder = {
        .home      = 0, // c:\Users\<username>
        .desktop   = 1,
        .documents = 2,
        .downloads = 3,
        .music     = 4,
        .pictures  = 5,
        .videos    = 6,
        .shared    = 7, // c:\Users\Public
        .bin       = 8, // c:\Program Files
        .data      = 9  // c:\ProgramData
    },
    // methods:
    .open         = rt_files_open,
    .is_valid     = rt_files_is_valid,
    .seek         = rt_files_seek,
    .stat         = rt_files_stat,
    .read         = rt_files_read,
    .write        = rt_files_write,
    .flush        = rt_files_flush,
    .close        = rt_files_close,
    .write_fully  = rt_files_write_fully,
    .exists       = rt_files_exists,
    .is_folder    = rt_files_is_folder,
    .is_symlink   = rt_files_is_symlink,
    .mkdirs       = rt_files_mkdirs,
    .rmdirs       = rt_files_rmdirs,
    .create_tmp   = rt_files_create_tmp,
    .chmod777     = rt_files_chmod777,
    .unlink       = rt_files_unlink,
    .link         = rt_files_link,
    .symlink      = rt_files_symlink,
    .basename     = rt_files_basename,
    .copy         = rt_files_copy,
    .move         = rt_files_move,
    .cwd          = rt_files_cwd,
    .chdir        = rt_files_chdir,
    .known_folder = rt_files_known_folder,
    .bin          = rt_files_bin,
    .data         = rt_files_data,
    .tmp          = rt_files_tmp,
    .opendir      = rt_files_opendir,
    .readdir      = rt_files_readdir,
    .closedir     = rt_files_closedir,
    .test         = rt_files_test
};
