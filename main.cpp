#include <iostream>
#include <unordered_map>
#include <unordered_set>
// #include <queue>
#include <list>
#include <vector>
#include <cstring>
#include <string>
#include <algorithm>

#include "../../Components/pugixml/src/pugixml.hpp"

using namespace std;

struct IdentifiedObject
{
    string aliasName;
    string description;
    string name;

    IdentifiedObject() : aliasName(""), description(""), name("") {}
};

struct EnergyEntity
{
    string id;
    string type;
    IdentifiedObject identifi;

    bool isConducting = false;
    bool isSwitch = false;
    bool switchPosition = false;

    unordered_set<string> links;

    EnergyEntity() : id(""), type(""), identifi(IdentifiedObject()), links({}) {}
};

struct TreeNode
{
    EnergyEntity *entity;
    unordered_set<TreeNode *> links;

    TreeNode() : entity(nullptr), links({}) {}
    TreeNode(EnergyEntity *ent) : entity(ent), links({}) {}
};

unordered_map<string, EnergyEntity *> cimObjects;
unordered_map<string, TreeNode *> cimNodes;

void checkoutOneLine(TreeNode *node, string mainNodeId)
{
    bool turnOffLine = false, findOffSwitch = false;

    list<TreeNode *> needVisit;
    needVisit.push_back(node);

    unordered_set<string> visited;
    visited.insert(node->entity->id);

    while (!needVisit.empty())
    {
        // Проверим все узлы текущего уровня
        for (int qS = needVisit.size(); qS > 0; qS--)
        {
            auto curNode = needVisit.front();
            needVisit.pop_front();
            visited.insert(curNode->entity->id);

            if (curNode->entity->isSwitch && !curNode->entity->switchPosition && curNode->entity->type.compare("GroundDisconnector") != 0)
            {
                // проверим можно ли пройти по другому ребру
                bool haveEdgeToContinue = false;
                for (auto qN : needVisit)
                {
                    if (qN->links.empty())
                        continue;

                    for (auto qNChild : qN->links)
                    {
                        if (!visited.count(qNChild->entity->id) && !qNChild->links.empty())
                        {
                            haveEdgeToContinue = true;
                            break;
                        }
                    }
                    if (haveEdgeToContinue)
                        break;
                }

                if (!haveEdgeToContinue)
                {
                    cout << "Find turn off switch " << curNode->entity->id << " = " << curNode->entity->type << " = " << curNode->entity->identifi.name << endl;
                    findOffSwitch = true;
                    // Turn off line
                    // отключаем только если упираемся в выключатель
                    turnOffLine = true;
                    needVisit.clear();
                    qS = 0;
                    break;
                }
            }

            for (auto child : curNode->links)
            {
                bool noLinks = true;
                if (!visited.count(child->entity->id))
                {
                    if (child->entity->id.compare(mainNodeId) != 0)
                    {
                        needVisit.push_back(child);
                        noLinks = false;
                    }
                }
                if (child->entity->type.find("Plant") != string::npos)
                {
                    cout << "Find Plant, line with voltage. " << child->entity->identifi.name << " = " << child->entity->id << endl;
                    needVisit.clear();
                    break;
                }

            } // for (auto child : curNode->links)

        } // for (int qS = needVisit.size(); qS > 0; qS--)

        if (findOffSwitch)
            break;

    } // while (!needVisit.empty())

    if (turnOffLine)
        cout << "Result: Turn off line " << node->entity->type << " = " << node->entity->id << endl;
    else
        cout << "Result: Line with voltage " << node->entity->type << " = " << node->entity->id << endl;

    cout << "Visited nodes:" << endl;
    for_each(visited.begin(), visited.end(), [](auto one)
             { cout << one << " = " << cimNodes.at(one)->entity->type << " - " << cimNodes.at(one)->entity->identifi.name << endl; });
}

void turnOffTrace(string rdfId)
{
    if (cimNodes.find(rdfId) == cimNodes.end())
    {
        cout << "This id does not exist" << endl;
        return;
    }
    auto root = cimNodes.at(rdfId);
    cout << "Start from " << root->entity->type << " with name " << root->entity->identifi.name << endl;

    if (root->links.empty())
    {
        cout << "Object does not have links";
        return;
    }

    for (auto link : root->links)
    {
        cout << endl
             << "CHECK MAIN CHILD: " << link->entity->id << " " << link->entity->type << " - " << link->entity->identifi.name << endl;
        checkoutOneLine(link, root->entity->id);
    }
}

int main(int argc, char *argv[])
{
    printf("locale: %s\n", std::setlocale(LC_ALL, NULL));

#ifdef LINUX
    if (0 /*std::setlocale(LC_ALL, "ru_RU.cp1251")*/)
#else
    if (std::setlocale(LC_ALL, "Russian_Russia.1251")) // "rus" or "Russian_Russia.1251"
#endif
        printf("set locale: %s\n", std::setlocale(LC_ALL, NULL));
    else
        printf("error set locale to Russia.1251\n");

    std::setlocale(LC_NUMERIC, "C");

    printf("-------------------------------------------------\n");
    printf("  CIM/RDF Parser\n");
#if defined __x86_64__ && !defined __ILP32__
    printf("  Build:  %s  %s, version x64\n", __DATE__, __TIME__);
#else
    printf("  Build:  %s  %s, version x32\n", __DATE__, __TIME__);
#endif
    printf("  GCC version:  %s\n", __VERSION__);
    printf("-------------------------------------------------\n");

    string binPath = argv[0];
    // string xmlPath = argv[1];
    string xmlPath = "C:\\ENTEK\\source\\CimModel\\trenenergo-small-new.rdf";

    printf("Bin path %s\n", binPath.c_str());
    printf("Path to CIM file %s\n", xmlPath.c_str());

    pugi::xml_document doc;

    if (!doc.load_file(xmlPath.c_str()))
    {
        cout << "Error while loading xml";
        return 1;
    }

    pugi::xml_node mainNode;

    mainNode = doc.first_child();
    if (!mainNode)
    {
        cout << "Error while loading xml";
        return 1;
    }

    // Parse rdf
    for (pugi::xml_node cimNode = mainNode.first_child(); cimNode; cimNode = cimNode.next_sibling())
    {
        EnergyEntity *ent = new EnergyEntity();
        string tmp = cimNode.name();
        tmp = tmp.substr(tmp.find_first_of(':') + 1);
        ent->type = tmp;
        ent->id = cimNode.attribute("rdf:ID").value();
        ent->identifi.aliasName = cimNode.child_value("cim:IdentifiedObject.aliasName");
        ent->identifi.description = cimNode.child_value("cim:IdentifiedObject.description");
        ent->identifi.name = cimNode.child_value("cim:IdentifiedObject.name");

        vector<string> tmpLinks;
        string tmpRes;
        if (cimNode.child("cim:Terminal.ConnectivityNode"))
        {
            tmpRes = cimNode.child("cim:Terminal.ConnectivityNode").attribute("rdf:resource").value();
            if (!tmpRes.empty())
                tmpLinks.push_back(tmpRes.substr(1));
            tmpRes.clear();
        }

        if (cimNode.child("cim:Terminal.ConductingEquipment"))
        {
            tmpRes = cimNode.child("cim:Terminal.ConductingEquipment").attribute("rdf:resource").value();
            if (!tmpRes.empty())
                tmpLinks.push_back(tmpRes.substr(1));
            tmpRes.clear();
        }

        if (cimNode.child("cim:TransformerWinding.MemberOf_PowerTransformer"))
        {
            tmpRes = cimNode.child("cim:TransformerWinding.MemberOf_PowerTransformer").attribute("rdf:resource").value();
            if (!tmpRes.empty())
                tmpLinks.push_back(tmpRes.substr(1));
            tmpRes.clear();
        }

        if (cimNode.child("cim:Equipment.MemberOf_EquipmentContainer"))
        {
            tmpRes = cimNode.child("cim:Equipment.MemberOf_EquipmentContainer").attribute("rdf:resource").value();
            if (!tmpRes.empty())
                tmpLinks.push_back(tmpRes.substr(1));
            tmpRes.clear();
        }

        if (cimNode.child("cim:Switch.normalOpen"))
        {
            ent->isSwitch = true;
            ent->switchPosition = !cimNode.child("cim:Switch.normalOpen").text().as_bool(false);
        }

        if (cimObjects.find(ent->id) != cimObjects.end())
        {
            if (!tmpLinks.empty())
            {
                // cimObjects.at(ent->id)->links.insert(tmpLinks);
                std::copy(tmpLinks.begin(), tmpLinks.end(),
                          std::inserter(cimObjects.at(ent->id)->links, cimObjects.at(ent->id)->links.end()));
            }
        }
        else
        {
            if (!tmpLinks.empty())
                std::copy(tmpLinks.begin(), tmpLinks.end(), std::inserter(ent->links, ent->links.end()));
            cimObjects.insert({ent->id, ent});
        }

        cimNodes.insert({ent->id, new TreeNode(ent)});
    }

    cout << "Parsed " << cimObjects.size() << " objects" << endl;

    // Make graph
    for (auto node : cimNodes)
    {
        if (node.second->entity->links.size() > 0)
        {
            for (auto link : node.second->entity->links)
            {
                if (cimNodes.find(link) != cimNodes.end() && cimNodes.at(link)->entity->type.find("Voltage") == string::npos && cimNodes.at(link)->entity->type.find("Bay") == string::npos)
                {
                    node.second->links.insert(cimNodes.at(link));
                    cimNodes.at(link)->links.insert(node.second);
                }
            }
        }
    }
    cout << "Graph is ready" << endl;

    string rdfId;
    cout << "Write rdf:id" << endl;
    cin >> rdfId;
    while (rdfId.compare("0") != 0)
    {
        turnOffTrace(rdfId);
        cout << endl
             << "Write rdf:id" << endl;
        cin >> rdfId;
    }

    return 0;
}