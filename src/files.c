#include "runtime.h"
#include "win32.h"

begin_c

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
        r = GetLastError();
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
        }
        if (transferred != null) { *transferred = written; }
        errno_t rc = CloseHandle(file);
        if (r == 0) { r = rc; }
    }
    return r;
}

static errno_t files_remove_file_or_folder(const char* pathname) {
    if (files.is_folder(pathname)) {
        return b2e(RemoveDirectoryA(pathname));
    } else {
        return b2e(DeleteFileA(pathname));
    }
}

static errno_t files_tmp(char* folder, int32_t count) {
    // create temporary folder
    assert(folder != null && count > 0);
    char temp_path[1024] = { 0 };
    // If GetTempPathA() succeeds, the return value is the length,
    // in chars, of the string copied to lpBuffer, not including
    // the terminating null character.
    errno_t r = b2e(GetTempPathA(countof(temp_path), temp_path));
    if (r != 0) { traceln("GetTempPathA() failed %s", str.error(r)); }
    if (count < (int)strlen(temp_path) + 8) {
        r = ERROR_BUFFER_OVERFLOW;
    }
    if (r == 0) {
        assert(count > (int)strlen(temp_path) + 8);
        // If GetTempFileNameA() succeeds, the return value is the length,
        // in chars, of the string copied to lpBuffer, not including the
        // terminating null character.
        if (count > (int)strlen(temp_path) + 8) {
            char prefix[4] = {0};
            r = b2e(GetTempFileNameA(temp_path, prefix, 0, folder));
            if (r != 0) { traceln("GetTempFileNameA() failed %s", str.error(r)); }
        } else {
            r = ERROR_BUFFER_OVERFLOW;
        }
    }
    if (r == 0) {
        r = b2e(DeleteFileA(folder));
        if (r != 0) { traceln("DeleteFileA(%s) failed %s", folder, str.error(r)); }
    }
    if (r == 0) {
        r = files.mkdirs(folder);
        if (r != 0) { traceln("mkdirs(%s) failed %s", folder, str.error(r)); }
    }
    return r;
}

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
    uint32_t bytes_needed = sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(sid) - sizeof(DWORD);
    errno_t r = b2e(GetAclInformation(acl, &info, sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation));
    if (r == 0 && info.AclBytesFree < bytes_needed) {
        const int64_t bytes = info.AclBytesInUse + bytes_needed;
        bigger = (ACL*)mem.heap.allocate(null, bytes, true);
        if (bigger == null) {
            r = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            r = b2e(InitializeAcl(bigger,
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
        ACCESS_ALLOWED_ACE* ace = (ACCESS_ALLOWED_ACE*)
            zero_initialized_stackalloc(bytes_needed);
        ace->Header.AceFlags = flags;
        ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        ace->Header.AceSize = (WORD)bytes_needed;
        ace->Mask = mask;
        ace->SidStart = sizeof(ACCESS_ALLOWED_ACE);
        memcpy(&ace->SidStart, sid, GetLengthSid(sid));
        r = b2e(AddAce(bigger != null ? bigger : acl, ACL_REVISION, MAXDWORD,
                       ace, bytes_needed));
    }
    *free_me = bigger;
    return r;
}

/* This is handy for debugging
static errno_t print_ace(ACCESS_ALLOWED_ACE* ace) {
    SID* sid = (SID*)&ace->SidStart;
    DWORD l1 = 128, l2 = 128;
    char account[128];
    char group[128];
    SID_NAME_USE use;
    errno_t r = b2e(LookupAccountSidA(null, sid, account, &l1, group, &l2, &use));
    if (r == 0) {
        log_info("%s/%s: type: %d, mask: 0x%X, flags:%d", group, account, ace->Header.AceType, ace->Mask, ace->Header.AceFlags);
    }
    log_flush();
    return r;
}
*/

static errno_t files_add_acl_ace(const void* obj, int32_t obj_type,
                                 int32_t sid_type, uint32_t mask) {
    int32_t n = SECURITY_MAX_SID_SIZE;
    SID* sid = (SID*)zero_initialized_stackalloc(n);
    errno_t r = b2e(CreateWellKnownSid((WELL_KNOWN_SID_TYPE)sid_type,
                                       null, sid, (DWORD*)&n));
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
                // traceln("updating existing ace");
                found->Mask |= mask;
                r = files_set_acl(obj, obj_type, acl);
            } else {
                // traceln("desired access is already allowed by ace");
            }
        } else if (r == 0) {
            // traceln("inserting new ace");
            ACL* new_acl = null;
            byte flags = obj_type == SE_FILE_OBJECT ?
                CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE : 0;
            r = files_acl_add_ace(acl, sid, mask, &new_acl, flags);
            if (r == 0) {
                r = files_set_acl(obj, obj_type, (new_acl != null ? new_acl : acl));
            }
            if (new_acl != null) { mem.heap.deallocate(null, new_acl); }
        }
    }
    if (sd != null) { LocalFree(sd); }
    return r;
}

static errno_t files_chmod777(const char* pathname) {
    errno_t r = 0;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID everyone = null; // Create a well-known SID for the Everyone group.
    BOOL b = AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID,
             0, 0, 0, 0, 0, 0, 0, &everyone);
    assert(b && everyone != null);
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
    b = b && SetEntriesInAclA(1, ea, null, &acl) == ERROR_SUCCESS;
    assert(b && acl != null);
    // Initialize a security descriptor.
    SECURITY_DESCRIPTOR* sd = (SECURITY_DESCRIPTOR*)
        stackalloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    b = InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    assert(b);
    // Add the ACL to the security descriptor.
    b = b && SetSecurityDescriptorDacl(sd, /* bDaclPresent flag: */ true,
                                   acl, /* not a default DACL: */  false);
    assert(b);
    // Change the security attributes
    b = b && SetFileSecurityA(pathname, DACL_SECURITY_INFORMATION, sd);
    if (!b) {
        r = runtime.err();
        traceln("chmod777(%s) failed %s", pathname, str.error(r));
    }
    if (everyone != null) { FreeSid(everyone); }
    if (acl != null) { LocalFree(acl); }
    return r;
}

static const char* files_get_program_files(void) {
    static char program_files[MAX_PATH];
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

static const char* files_get_program_data(void) {
    static char program_data[MAX_PATH];
    if (program_data[0] == 0) {
        wchar_t* program_data_w = null;
        fatal_if(SHGetKnownFolderPath(&FOLDERID_ProgramData, 0,
            null, &program_data_w) != 0);
        int32_t len = (int32_t)wcslen(program_data_w);
        assert(len < countof(program_data));
        fatal_if(len >= countof(program_data), "len=%d", len);
        for (int32_t i = 0; i <= len; i++) { // including zero terminator
            assert(program_data_w[i] < 128); // pure ascii
            program_data[i] = (char)program_data_w[i];
        }
    }
    return program_data;
}

// https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createdirectorya
// "If lpSecurityAttributes is null, the directory gets a default security
//  descriptor. The ACLs in the default security descriptor for a directory
//  are inherited from its parent directory."

static errno_t files_mkdirs(const char* dir) {
    errno_t r = 0;
    const int32_t n = (int)strlen(dir) + 1;
    char* s = (char*)stackalloc(n);
    memset(s, 0, n);
    const char* next = strchr(dir, '\\');
    if (next == null) { next = strchr(dir, '/'); }
    while (next != null) {
        if (next > dir && *(next - 1) != ':') {
            memcpy(s, dir, next - dir);
            r = b2e(CreateDirectoryA(s, null));
            if (r != 0 && r != ERROR_ALREADY_EXISTS) { break; }
        }
        const char* prev = ++next;
        next = strchr(prev, '\\');
        if (next == null) { next = strchr(prev, '/'); }
    }
    if (r == 0 || r == ERROR_ALREADY_EXISTS) {
        r = b2e(CreateDirectoryA(dir, null));
    }
    return r == ERROR_ALREADY_EXISTS ? 0 : r;
}

static errno_t files_rmdirs(const char* folder) {
    folders_t* dirs = folders.open();
    errno_t r = dirs == null ?
            ERROR_OUTOFMEMORY : folders.enumerate(dirs, folder);
    if (r == 0) {
        const int32_t n = folders.count(dirs);
        for (int32_t i = 0; i < n; i++) {
            // recurse into sub folders and remove them first
            // do NOT follow symlinks - it could be disastrous
            if (!folders.is_symlink(dirs, i) && folders.is_folder(dirs, i)) {
                const char* name = folders.name(dirs, i);
                int32_t k = (int32_t)(strlen(folder) + strlen(name) + 3);
                char* pathname = (char*)mem.heap.allocate(null, k, false);
                if (pathname == null) {
                    r = ERROR_OUTOFMEMORY;
                    break;
                }
                str.sformat(pathname, k, "%s/%s", folder, name);
                r = files.rmdirs(pathname);
                mem.heap.deallocate(null, pathname);
                if (r != 0) { break; }
            }
        }
        for (int32_t i = 0; i < n; i++) {
            if (!folders.is_folder(dirs, i)) { // symlinks are removed as normal files
                const char* name = folders.name(dirs, i);
                int32_t k = (int32_t)(strlen(folder) + strlen(name) + 3);
                char* pathname = (char*)mem.heap.allocate(null, k, false);
                if (pathname == null) {
                    r = ERROR_OUTOFMEMORY;
                    break;
                }
                str.sformat(pathname, k, "%s/%s", folder, name);
                r = files.unlink(pathname);
                if (r != 0) {
                    traceln("remove(%s) failed %s", pathname, str.error(r));
                }
                mem.heap.deallocate(null, pathname);
                if (r != 0) { break; }
            }
        }
    }
    if (dirs != null) { folders.close(dirs); }
    if (r == 0) { r = files.unlink(folder); }
    return r;
}

static bool files_exists(const char* path) {
    return PathFileExistsA(path);
}

static bool files_is_folder(const char* path) {
    return PathIsDirectoryA(path);
}

static const char* files_cwd(char* wd, int32_t count) {
    assert(count > 0);
    if (count <= 0) {
        return null;
    } else {
        DWORD bytes = count - 1;
        if (GetCurrentDirectoryA(bytes, wd)) {
            wd[count - 1] = 0;
            return wd;
        } else {
            return null;
        }
    }
}

static errno_t files_setcwd(const char* wd) {
    return b2e(SetCurrentDirectoryA(wd));
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
    DWORD flags = files.is_folder(from) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
    return b2e(CreateSymbolicLinkA(from, to, flags));
}

static void files_test(void) {
    #ifdef RUNTIME_TESTS
    traceln("TODO");
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
    #endif
}

files_if files = {
    .write_fully        = files_write_fully,
    .exists             = files_exists,
    .is_folder          = files_is_folder,
    .mkdirs             = files_mkdirs,
    .rmdirs             = files_rmdirs,
    .tmp                = files_tmp,
    .chmod777           = files_chmod777,
    .folder_bin         = files_get_program_files,
    .folder_data        = files_get_program_data,
    .unlink             = files_remove_file_or_folder,
    .link               = files_link,
    .symlink            = files_symlink,
    .cwd                = files_cwd,
    .copy               = files_copy,
    .move               = files_move,
    .setcwd             = files_setcwd,
    .test               = files_test
};

end_c

