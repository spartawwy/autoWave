#include "file_mapping.h"

#include <ctime>
#include <direct.h>
#include <iostream>
#include <Windows.h>

#define BAD_POS 0xFFFFFFFF // returned by SetFilePointer and GetFileSize
#define SUCCESS 0

//using namespace std; 
FileMapping::FileMapping() : mmfm_base_address_(nullptr), mmfm_(INVALID_HANDLE_VALUE), mmHandle_(INVALID_HANDLE_VALUE)
    , file_size_(0), mmf_size_(1024*1024), view_size_(1000*1024)
{

}

FileMapping::~FileMapping()
{
    UnmapViewOfFile(mmfm_base_address_);
     
    if( mmfm_ != INVALID_HANDLE_VALUE )
        CloseHandle(mmfm_);
    // close file handle
    if( mmHandle_ != INVALID_HANDLE_VALUE )
        CloseHandle(mmHandle_);
}

bool FileMapping::Create(const std::string & file_full_path)
{ 
    bool is_allow_write = true;
    DWORD error_code;
    //TRetCookie  ret = TRetCookie::OK;

    std::string tag_str; 
    auto pos = file_full_path.rfind("/");
    auto pos1 = file_full_path.rfind("\\");
    assert( pos != file_full_path.size() - 1);

    if( pos != std::string::npos 
        && file_full_path.substr(pos + 1, file_full_path.size() - pos - 1).find("\\") == std::string::npos )
        tag_str = file_full_path.substr(pos + 1, file_full_path.size() - pos - 1);
    else if( pos1 != std::string::npos )
        tag_str = file_full_path.substr(pos1 + 1, file_full_path.size() - pos1 - 1);
    else
        tag_str = "sfwdfsfilemp";
    std::srand(time(0));
    tag_str += std::to_string(std::rand());

    char shared_name[256] = {0};
    strcpy_s(shared_name, tag_str.c_str());
    char * rep_p = strstr(shared_name, ".");
    if( rep_p ) *rep_p = '_';
    //const char* shared_name = "cookietrdw_mock";
       
    mmfm_ = INVALID_HANDLE_VALUE;
    // create file
    int access_mode = GENERIC_READ;
    int shared_mode = FILE_SHARE_READ;
    if( is_allow_write )
    {
        access_mode |= GENERIC_WRITE;
        shared_mode |= FILE_SHARE_WRITE;
    }
    mmHandle_ = CreateFile(const_cast<char*>(file_full_path.c_str()),
             access_mode,  //access_mode
             shared_mode, //share_mode
             NULL,
             OPEN_ALWAYS,
             FILE_FLAG_SEQUENTIAL_SCAN,
             NULL);
 
    if (mmHandle_ == INVALID_HANDLE_VALUE) 
    {
       error_code = GetLastError();
       //ret = TRetCookie::ERROR_OTHER;
        //  throw exception
       goto EXIT_PRO;
        
    } 
     
    DWORD high_size;
    file_size_ = GetFileSize(mmHandle_, &high_size);
    if (file_size_ == BAD_POS && (error_code = GetLastError()) != SUCCESS)
    {
        //ret = TRetCookie::ERROR_OTHER;
        goto EXIT_PRO;
    }
    mmf_size_ = (file_size_ / 1024) * 1024 + (file_size_ % 1024 > 0 ? 1024 : 0);
    view_size_ = mmf_size_;
    DWORD size_high = 0;
    int page_mode = PAGE_READONLY;
    if( is_allow_write )
        page_mode = PAGE_READWRITE;
    //创建文件映射，如果要创建内存页面文件的映射，第一个参数设置为INVALID_HANDLE_VALUE
    mmfm_ = CreateFileMapping(mmHandle_,
                                    NULL,
                                    page_mode,
                                    size_high,
                                    mmf_size_,
                                    shared_name);
 
    error_code = GetLastError();
    if(SUCCESS != error_code || mmfm_ == NULL )   
    {             
        //ret = TRetCookie::ERROR_FILE_OPEN; 
        goto EXIT_PRO;
    } 
    int map_access_mode = FILE_MAP_READ;
    if( is_allow_write )
        map_access_mode |= FILE_MAP_WRITE;               
    mmfm_base_address_ = (char*)MapViewOfFile(mmfm_, map_access_mode, 0/*fileofsethigh*/, 0/*fileofsetlow*/, view_size_);
    if(mmfm_base_address_ == NULL)
    {
        error_code = GetLastError();
        if(error_code != SUCCESS)
        {
            std::cout << "error code " << error_code << std::endl;
        }
        //ret = TRetCookie::ERROR_FILE_OPEN;
        goto EXIT_PRO;

    } 

    //data_ = reinterpret_cast<T_DataAccess*>(mmfm_base_address_);
    //mmfm_base_address_[file_size_] = '\0';     
    return true;
 
EXIT_PRO:
     
    if( mmfm_base_address_ != INVALID_HANDLE_VALUE && mmfm_base_address_ != NULL )
        UnmapViewOfFile(mmfm_base_address_);
    if( mmfm_ != INVALID_HANDLE_VALUE && mmfm_ != NULL )
        CloseHandle(mmfm_);
    mmfm_ = INVALID_HANDLE_VALUE;

    if( mmHandle_ != INVALID_HANDLE_VALUE && mmHandle_ != NULL )
        CloseHandle(mmHandle_);
    mmHandle_ = INVALID_HANDLE_VALUE;
    mmfm_base_address_ = NULL;
    return false;
}

char * FileMapping::data_address() 
{ 
    if( mmfm_ == INVALID_HANDLE_VALUE )
        return nullptr;
    return mmfm_base_address_; 
}

unsigned long FileMapping::total_line()
{
    unsigned long line = 0;
    const unsigned long max_len = file_size_;
    unsigned long len = 0;
    unsigned long real_len = 0;

    //char *p_start = mmfm_base_address_ + len;
    while( len < max_len )
    {
        char *p = mmfm_base_address_ + len;
        while( len < max_len && ((int)*p != 0x0D && (int)*p != 0x0A) )
        {
            ++p;
            ++len;
            ++real_len;
        }

        if( len < max_len )
            ++len; // to pass '\r' or '\n'
        if( len < max_len && (int)*(p+1) == 0x0A)
            ++len; // to pass '\n''

        ++line;
    }
    return line;
}

// return start pos;  if -1 fail
unsigned long FileMapping::find_line_start(unsigned long line)
{
    unsigned long line_index = 0;
    const unsigned long max_len = file_size_;
    unsigned long len = 0;
    unsigned long real_len = 0;

    //char *p_start = mmfm_base_address_ + len;
    while( len < max_len )
    {
        if( line_index == line )
            return len;
        char *p = mmfm_base_address_ + len;
        while( len < max_len && ((int)*p != 0x0D && (int)*p != 0x0A) )
        {
            ++p;
            ++len;
            ++real_len;
        }

        if( len < max_len )
            ++len; // to pass '\r' or '\n'
        if( len < max_len && (int)*(p+1) == 0x0A)
            ++len; // to pass '\n''

        ++line_index;
    }
    return BAD_POS;
}

// ret: walked len.  ret_str mustn't have '\r\n' or '\n'
// ps: walked len may != ret_str.size()
unsigned long FileMapping::get_line(unsigned long start_pos, std::string & ret_str)
{
    /* 由于Linux用\n=0x0A=10=LF来表示换行，所以，打开Windows中的文件的时候，如果其中有换行，
    即其中有\r\n= 0x0D 0x0A，此时，就会被处理为，
    只将n理解为换行，而把r看作为一个单独的字符，此r字符，对应ASCII的值是0x0D=13,是个控制字符
    */
    ret_str.clear();
    if( !mmfm_base_address_ )
        return 0;
    if( start_pos >= file_size_ )
        return 0;
    const unsigned long max_len = file_size_ - start_pos;
    unsigned long len = 0;
    unsigned long real_len = 0;

    char *p_start = mmfm_base_address_ + start_pos;
    char *p = p_start;
    while( len < max_len && ((int)*p != 0x0D && (int)*p != 0x0A) )
    {
        ++p;
        ++len;
        ++real_len;
    }

    if( len < max_len )
        ++len; // to pass '\r' or '\n'
    if( len < max_len && (int)*(p+1) == 0x0A)
        ++len; // to pass '\n''

    
    if( len > 0 )
    {
#if 1
        if( real_len > 0)
        {
            char buf[1025 * 5] = {'\0'};
            assert(len < sizeof(buf));
            memcpy(buf, mmfm_base_address_ + start_pos, real_len);
            ret_str = buf;
        }
#else 
        // filter '\r' and '\n'
        while( p > p_start && ((int)*p == 0x0D || (int)*p == 0x0A) )
        {
            --p; 
        }
        std::string src(p_start, p);
        ret_str = src;
#endif
    }
    return len;
}