#include "McmRegistryPersistence.h"
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <string>

static void put(const wchar_t* file, const char* text) {
 HANDLE h=CreateFileW(file,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(h==INVALID_HANDLE_VALUE){printf("create fail %lu\n",GetLastError());exit(1);}
 DWORD n;DWORD len=(DWORD)strlen(text);
 if(!WriteFile(h,text,len,&n,nullptr)||n!=len){printf("write failed\n");exit(2);}CloseHandle(h);
}
static std::string get(const wchar_t* file) {
 HANDLE h=CreateFileW(file,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(h==INVALID_HANDLE_VALUE){printf("open fail %lu\n",GetLastError());exit(3);}
 std::string bytes;
 if(!k2040::ReadMcmRegistryHandle(h,bytes)){printf("read failed\n");exit(4);}
 CloseHandle(h);return bytes;
}
static void test(bool lock) {
 const std::wstring dest = lock ? L"persistence-locked.json" : L"persistence-free.json";
 const std::wstring temp = dest + L".k2040.tmp";
 const std::wstring backup = dest + L".k2040-backup";
 DeleteFileW(backup.c_str());
 put(dest.c_str(),"{\"keybinds\":[1]}");put(temp.c_str(),"{\"keybinds\":[2]}");
 HANDLE handle=INVALID_HANDLE_VALUE;
 if(lock) {
  handle=CreateFileW(dest.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(handle==INVALID_HANDLE_VALUE){printf("lock failed: %lu\n",GetLastError());exit(5);}
 }
 auto result=k2040::SaveSharedMcmRegistry(temp,dest,"{\"keybinds\":[1]}","{\"keybinds\":[2]}");
 if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);
 printf("lock=%d saved=%d fallback=%d moveError=%lu fallbackError=%lu restoreFailed=%d\n",lock,result.saved,result.usedInPlaceFallback,result.replaceError,result.fallbackError,result.restoreFailed);
 if(!result.saved || get(dest.c_str())!="{\"keybinds\":[2]}") exit(6);
 if(lock){if(!result.usedInPlaceFallback || get(backup.c_str())!="{\"keybinds\":[1]}")exit(7);}
 else if(result.usedInPlaceFallback)exit(8);
}
static void changedSourceMustNotBeOverwritten() {
 const std::wstring dest=L"persistence-stale.json";
 const std::wstring temp=dest+L".k2040.tmp";
 put(dest.c_str(),"{\"keybinds\":[3]}");
 put(temp.c_str(),"{\"keybinds\":[2]}");
 const HANDLE lock=CreateFileW(dest.c_str(),GENERIC_READ,
     FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(lock==INVALID_HANDLE_VALUE)exit(9);
 const auto result=k2040::SaveSharedMcmRegistry(temp,dest,
     "{\"keybinds\":[1]}","{\"keybinds\":[2]}");
 CloseHandle(lock);
 if(result.saved || get(dest.c_str())!="{\"keybinds\":[3]}") exit(10);
}
int main(){
 test(false);test(true);changedSourceMustNotBeOverwritten();
 printf("PASS: Win32 normal atomic save, locked-file recoverable fallback, stale-data rejection\n");
 return 0;
}
