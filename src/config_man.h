#ifndef CONFIG_MAN_SDFKKDSFKDSJFK_H_
#define CONFIG_MAN_SDFKKDSFKDSJFK_H_
#include <string>
#include <unordered_map>
#include <memory>

#define DEFAULT_CODE  "SC1904"

struct ContractCfgInfo
{
    std::string code;
    int eldest_date;
    int eldest_hhmm;
    int latest_date;
    int latest_hhmm;
    int end_date;
};

//struct StrategyCfgAtom
//{
//    std::string name; 
//};
struct StrategyCfgInfo
{
    std::string name;  
};

class ConfigMan
{
public:
    ConfigMan();

    bool LoadConfig(const std::string &cfg_file_path);
    const ContractCfgInfo & contract_info() { return contract_info_; }

private:
    ContractCfgInfo contract_info_;
    //<name, info>
    std::unordered_map<std::string, std::shared_ptr<StrategyCfgInfo> > strategy_cfg_info_;
};

#endif // CONFIG_MAN_SDFKKDSFKDSJFK_H_