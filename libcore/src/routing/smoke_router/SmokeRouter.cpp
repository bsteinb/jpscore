/**
 * \file        SmokeRouter.cpp
 * \date        Feb 1, 2014
 * \version     v0.7
 * \copyright   <2009-2015> Forschungszentrum Jülich GmbH. All rights reserved.
 *
 * \section License
 * This file is part of JuPedSim.
 *
 * JuPedSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * JuPedSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with JuPedSim. If not, see <http://www.gnu.org/licenses/>.
 *
 * \section Description
 *
 *
 **/
#include "SmokeRouter.h"

#include "BrainStorage.h"
#include "general/Logger.h"
#include "pedestrian/Pedestrian.h"
#include "sensor/SensorManager.h"

#include <tinyxml.h>

SmokeRouter::SmokeRouter()
{
    building = nullptr;
    //    cm_storage=nullptr;
    //    sensor_manager=nullptr;
}

SmokeRouter::SmokeRouter(int id, RoutingStrategy s) : Router(id, s)
{
    building = nullptr;
    //    cm_storage=nullptr;
    //    sensor_manager=nullptr;
}

SmokeRouter::~SmokeRouter()
{
    //delete brain_storage;
    delete sensor_manager;
}

int SmokeRouter::FindExit(Pedestrian * p)
{
    //check for former goal.
    if((*brain_storage)[p]->GetCognitiveMap().GetGraphNetwork()->HadNoDestination()) {
        sensor_manager->execute(p, SensorManager::INIT);
    }

    //Check if the Pedestrian already has a Dest. or changed subroom and needs a new one.
    if((*brain_storage)[p]->GetCognitiveMap().GetGraphNetwork()->ChangedSubRoom()) {
        //execute periodical sensors
        sensor_manager->execute(p, SensorManager::CHANGED_ROOM);

        int status = FindDestination(p);

        (*brain_storage)[p]->GetCognitiveMap().GetGraphNetwork()->UpdateSubRoom();

        return status;
    }

    // check if ped reached a hline
    if((*brain_storage)[p]->HlineReached()) {
        int status = FindDestination(p);

        //(*cm_storage)[p]->UpdateSubRoom();

        return status;
    }

    //std::cout << p->GetGlobalTime() << std::endl;
    //    if (std::fmod(p->GetGlobalTime(),sensor_manager->GetIntVPeriodicUpdate())==0.0 && p->GetGlobalTime()>0)
    //    {
    //        //Log->Write(std::to_string(p->GetGlobalTime()));
    //        sensor_manager->execute(p, SensorManager::PERIODIC);

    //        int status = FindDestination(p);

    //        //(*cm_storage)[p]->UpdateSubRoom();

    //        return status;

    //    }
    return 1;
}

int SmokeRouter::FindDestination(Pedestrian * p)
{
    // Discover doors
    sensor_manager->execute(p, SensorManager::NO_WAY);
    //check if there is a way to the outside the pedestrian knows (in the cognitive map)
    const GraphEdge * destination = nullptr;
    //Cognitive Map /Associations/ Waypoints/ landmarks

    //        (*cm_storage)[p]->UpdateMap();

    //--------------------COGMAP----------------------------
    //See if Landmarks are visible

    //(*brain_storage)[p]->GetCognitiveMap().UpdateMap();
    //Find next appropriate landmark
    //(*brain_storage)[p]->GetCognitiveMap().FindNextTarget();
    //Find appropriate door to reach next app. landmark
    //(*brain_storage)[p]->GetCognitiveMap().AssessDoors();
    //------------------------------------------------------

    //Log->Write(std::to_string((*cm_storage)[p]->GetOwnPos().GetX())+" "+std::to_string((*cm_storage)[p]->GetOwnPos().GetY()));

    destination = (*brain_storage)[p]->GetCognitiveMap().GetGraphNetwork()->GetDestination();
    if(destination == nullptr) {
        //no destination was found, now we could start the discovery!
        //1. run the no_way sensors for room discovery.
        sensor_manager->execute(p, SensorManager::NO_WAY);

        //check if this was enough for finding a global path to the exit

        destination = (*brain_storage)[p]->GetCognitiveMap().GetGraphNetwork()->GetDestination();

        if(destination == nullptr) {
            //we still do not have a way. lets take the "best" local edge
            //for this we don't calculate the cost to exit but calculate the cost for the edges at the actual vertex.
            destination =
                (*brain_storage)[p]->GetCognitiveMap().GetGraphNetwork()->GetLocalDestination();
        }
    }


    //if we still could not found any destination we are lost! Pedestrian will be deleted
    //no destination should just appear in bug case or closed rooms.
    if(destination == nullptr) {
        LOG_ERROR("Pedestrian {:d} was unable to find any destination", p->GetID());
        return -1;
    }

    (*brain_storage)[p]->GetCognitiveMap().GetGraphNetwork()->AddDestination(destination);
    sensor_manager->execute(p, SensorManager::NEW_DESTINATION);

    const Crossing * nextTarget = destination->GetCrossing();

    const NavLine * nextNavLine = (*brain_storage)[p]->GetNextNavLine(nextTarget);

    if(nextNavLine == nullptr) {
        LOG_ERROR("No visible next subtarget found. PED {:d}", p->GetID());
        return -1;
    }
    //setting crossing to ped
    p->SetExitLine(nextNavLine);
    p->SetExitIndex(nextNavLine->GetUniqueID());
    return 1;
}


bool SmokeRouter::Init(Building * b)
{
    LOG_INFO("Init SmokeRouter");
    building = b;

    LoadRoutingInfos(GetRoutingInfoFile());

    //Init Cognitive Map Storage, second parameter: decides whether cognitive Map is empty or complete
    if(getOptions().find("CognitiveMapFiles") == getOptions().end())
        brain_storage = std::shared_ptr<BrainStorage>(
            new BrainStorage(building, getOptions().at("CognitiveMap")[0]));
    else
        brain_storage = std::shared_ptr<BrainStorage>(new BrainStorage(
            building, getOptions().at("CognitiveMap")[0], getOptions().at("CognitiveMapFiles")[0]));
    LOG_INFO("CognitiveMapStorage initialized");

    //Init Sensor Manager
    sensor_manager = SensorManager::InitWithCertainSensors(b, brain_storage.get(), getOptions());
    LOG_INFO("SensorManager initialized");
    return true;
}


const optStorage & SmokeRouter::getOptions() const
{
    return options;
}

void SmokeRouter::addOption(const std::string & key, const std::vector<std::string> & value)
{
    options.insert(std::make_pair(key, value));
}

bool SmokeRouter::LoadRoutingInfos(const fs::path & filename)
{
    if(filename.empty())
        return true;

    LOG_INFO(
        "Loading extra routing information for the global/quickest path router from file {}",
        filename.string());

    TiXmlDocument docRouting(filename.string());
    if(!docRouting.LoadFile()) {
        LOG_ERROR("SmokeRouter, could not parse project file {}: \t%s", docRouting.ErrorDesc());
        return false;
    }

    TiXmlElement * xRootNode = docRouting.RootElement();
    if(!xRootNode) {
        LOG_ERROR("Root element not found");
        return false;
    }

    if(xRootNode->ValueStr() != "routing") {
        LOG_ERROR("Root element value is not 'routing'.");
        return false;
    }

    std::string version = xRootNode->Attribute("version");
    if(version < JPS_OLD_VERSION) {
        LOG_ERROR("Only version greater than {} supported", JPS_OLD_VERSION);
        return false;
    }
    int HlineCount = 0;
    for(TiXmlElement * xHlinesNode = xRootNode->FirstChildElement("Hlines"); xHlinesNode;
        xHlinesNode                = xHlinesNode->NextSiblingElement("Hlines")) {
        for(TiXmlElement * hline = xHlinesNode->FirstChildElement("Hline"); hline;
            hline                = hline->NextSiblingElement("Hline")) {
            double id      = xmltof(hline->Attribute("id"), -1);
            int room_id    = xmltoi(hline->Attribute("room_id"), -1);
            int subroom_id = xmltoi(hline->Attribute("subroom_id"), -1);

            double x1 = xmltof(hline->FirstChildElement("vertex")->Attribute("px"));
            double y1 = xmltof(hline->FirstChildElement("vertex")->Attribute("py"));
            double x2 = xmltof(hline->LastChild("vertex")->ToElement()->Attribute("px"));
            double y2 = xmltof(hline->LastChild("vertex")->ToElement()->Attribute("py"));

            Room * room       = building->GetRoom(room_id);
            SubRoom * subroom = room->GetSubRoom(subroom_id);

            //new implementation
            Hline * h = new Hline();
            h->SetID(id);
            h->SetPoint1(Point(x1, y1));
            h->SetPoint2(Point(x2, y2));
            h->SetRoom1(room);
            h->SetSubRoom1(subroom);

            if(building->AddHline(h)) {
                subroom->AddHline(h);
                HlineCount++;
                //h is freed in building
            } else {
                delete h;
            }
        }
    }
    LOG_INFO("Done loading extra routing information. Loaded {:d} Hlines", HlineCount);
    return true;
}

fs::path SmokeRouter::GetRoutingInfoFile()
{
    TiXmlDocument doc(building->GetProjectFilename().string());
    if(!doc.LoadFile()) {
        LOG_ERROR("GlobalRouter, could not parse project file: {}", doc.ErrorDesc());
        return "";
    }

    // everything is fine. proceed with parsing
    TiXmlElement * xMainNode  = doc.RootElement();
    TiXmlNode * xRouters      = xMainNode->FirstChild("route_choice_models");
    std::string nav_line_file = "";

    for(TiXmlElement * e = xRouters->FirstChildElement("router"); e;
        e                = e->NextSiblingElement("router")) {
        std::string strategy = e->Attribute("description");

        if(strategy == "AI") {
            if(e->FirstChild("parameters")) {
                if(e->FirstChild("parameters")->FirstChildElement("navigation_lines"))
                    nav_line_file = e->FirstChild("parameters")
                                        ->FirstChildElement("navigation_lines")
                                        ->Attribute("file");
            }
        }
    }

    if(nav_line_file == "")
        return nav_line_file;
    else
        return building->GetProjectRootDir() / nav_line_file;
}
