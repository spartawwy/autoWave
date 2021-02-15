#ifndef FILE_MAPPING_H_DSF3DSSDFFS_
#define FILE_MAPPING_H_DSF3DSSDFFS_
  
#include <string>
#include <cassert>

#define  BAD_POS (~0)

class FileMapping
{
public:

   /* enum class TRetCookie : char
    {
        OK = 0,
        ERROR_FILE_OPEN,
        ERROR_OTHER,
    };
    struct  T_DataAccess
    {
        int max_task_id;
        //std::string name;
    };*/

    FileMapping();
    ~FileMapping();
     
    bool Create(const std::string & file_full_path);
    
   /* void max_task_id(int id) { assert(data_); data_->max_task_id = id; }
    int max_task_id() { assert(data_); return data_->max_task_id; }*/
    char *data_address(); 
    unsigned long file_size() { return file_size_; }
    unsigned long get_line(unsigned long start_pos, std::string &ret_str);
    unsigned long find_line_start(unsigned long line);
    unsigned long total_line();

private:

     //T_DataAccess * data() { return data_; }

     char * mmfm_base_address_;
     void * mmfm_;
     void * mmHandle_;

     unsigned long file_size_;
     unsigned long mmf_size_;
     unsigned int view_size_;
      
};

#endif // FILE_MAPPING_H_DSF3DSSDFFS_