#include "config_man.h"
#include "rwini.h"


ConfigMan::ConfigMan()
{
    contract_info_.code = DEFAULT_CODE;
}

bool ConfigMan::LoadConfig(const std::string &cfg_file_path)
{
    Crwini cfg_file(cfg_file_path);
    std::string contract_code;
    cfg_file.ReadString("contract", "code", &contract_info_.code);
    contract_info_.eldest_date = cfg_file.ReadInt("contract", "eldest_date");
    contract_info_.eldest_hhmm = cfg_file.ReadInt("contract", "eldest_hhmm");
    contract_info_.latest_date = cfg_file.ReadInt("contract", "latest_date");
    contract_info_.latest_hhmm = cfg_file.ReadInt("contract", "latest_hhmm");
    contract_info_.end_date = cfg_file.ReadInt("contract", "end_date");
      
    if( contract_info_.eldest_date > contract_info_.latest_date 
        || (contract_info_.eldest_date == contract_info_.latest_date && contract_info_.eldest_hhmm > contract_info_.latest_hhmm ) )
        return false;
    return true;
}
