#include <iostream>
#include <string>
#include <print>
#include <filesystem>
#include <type_traits>
#include <concepts>

#include "EasyDB.hpp"

namespace fs = std::filesystem;

class XData : public EDB::DataObject {
public:
    int id;
    std::string name;
    double balance;

    XData(): id(0), balance(0){}
    XData(int _id, const std::string& _name, double _balance): id(_id), name(_name), balance(_balance){}
    XData(EDB::EasyDB::RowViewType rowView) {
        edb_load(rowView);
    }
    int edb_dump(EDB::EasyDB& edb) noexcept {
        if (edb.hascolumns({"ID", "Name", "Balance"})) {
            if (edb.hasrow(id)) {
                edb.update(id, "Name", name);
                edb.update(id, "Balance", balance);
            } else {
                edb.insert({id, name, balance});
            }
            return SUCCESS;
        }
        return COLUMN_NOT_FOUND;
    }
    int edb_load(EDB::ERowView rowView) noexcept {
        if (rowView.hascolumns({"ID", "Name", "Balance"})) {
            id = std::get<EDB::Int>(rowView["ID"]);
            name = std::get<EDB::Text>(rowView["Name"]);
            balance = std::get<EDB::Real>(rowView["Balance"]);
            return SUCCESS;
        }
        return COLUMN_NOT_FOUND;
    }
    void print() const {
        std::println("{} {} {}", id, name, balance);
    }
};

int main(int argc, char const *argv[])
{
    if (!fs::exists("XDataTest.db")) {
        EDB::EasyDB::create("XDataTest.db", 
                            "XDataTest", 
                            {{EDB::DataType::INT, "ID", 0}, {EDB::DataType::TEXT, "Name", 100}, {EDB::DataType::REAL, "Balance", 0}},
                            "ID");
    }

    EDB::EasyDB db;
    db.open("XDataTest.db");

    XData p1 = {55240622, "陈儒杰", 114.514};
    p1.print();
    db.dump(p1);

    XData p2;
    db.load(55240622, p2);
    p2.print();

    db.insert({55240001, "Alice", 520.1314});
    XData p3 = db.load<XData>(55240001);
    p3.print();
    
    db.close();
    return 0;
}
