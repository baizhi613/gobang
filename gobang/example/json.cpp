#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <jsoncpp/json/json.h>
std::string serialize()
{
    Json::Value root;
    root["姓名"]="小明";
    root["年龄"]=18;
    root["成绩"].append(98);
    root["成绩"].append(88.5);
    root["成绩"].append(78.5);
    Json::StreamWriterBuilder swb;
    Json::StreamWriter *sw=swb.newStreamWriter();
    std::stringstream ss;
    int ret=sw->write(root,&ss);
    if(ret!=0){
        std::cout<<"json serialize failed!!\n";
        return "";
    }
    std::cout<<ss.str()<<std::endl;
    delete sw;
    return ss.str();
}
void unserialize(const std::string &str)
{
    Json::CharReaderBuilder crb;
    Json::CharReader *cr=crb.newCharReader();
    Json::Value root;
    std::string err;
    bool ret=cr->parse(str.c_str(),str.c_str()+str.size(),&root,&err);
    if(ret==false){
        std::cout<<"json unserialize failed:"<<err<<std::endl;
        return ;
    }
    std::cout<<"姓名:"<<root["姓名"].asString()<<std::endl;
    std::cout<<"年龄:"<<root["年龄"].asInt()<<std::endl;
    int sz=root["成绩"].size();
    for(int i=0;i<sz;i++){
        std::cout<<"成绩:"<<root["成绩"][i].asFloat()<<std::endl;
    }
}
int main()
{
    std::string str=serialize();
    unserialize(str);
    return 0;
}