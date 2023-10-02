#include <iostream>
#include "GeneratorFacade.h"
#include <vector>
#include <string>
#include "json.hpp"
#include "mapgen/MapGenerator.hpp"
#include "noise/noise.h"
#include "noise/noiseutils.h"



using json = nlohmann::json;

json world;
char buff[70000000];
module::Perlin _perlin;
utils::NoiseMap _heightMap;
utils::NoiseMap _mineralsMap;
std::string _terrainType;
int _w;
int _h;
int _octaves;
float _freq;
int _seed;

MapGenerator* mapgen;

extern "C" {
        UNITY_LIB_API int createMap (int seed, int w, int h,int octave,float frequency,int pointCount, float seaLevel,const char* terrainType) 
        {
            _terrainType = std::string (terrainType);

            if (mapgen != nullptr)
            {
                delete mapgen;
            }
            

            mapgen = new MapGenerator(w, h);
            mapgen->setFrequency(frequency);
            mapgen->setMapTemplate(terrainType);
            mapgen->setOctaveCount(octave);
            mapgen->setPointCount(pointCount);
            mapgen->setSeed(seed);
            mapgen->setSeaLevel(seaLevel);
            mapgen->update();
    
            world = json({});
            auto regions = json::array();
            auto mClusters = json::array();
            auto clusters = json::array();
            auto rivers = json::array();
            
            int n = 0;

            for (int (k) = 0; (k) < mapgen->map->rivers.size(); ++(k)) 
            {
                auto r = mapgen->map->rivers[k];
                auto river_json = json({});
                river_json["id"] = k;
                river_json["name"] = r->name;
                auto points = r->points;
                
                auto f_points = json::array();
                int point_index = 0;
                for (PointList::iterator it2 = (*points).begin(); it2 < (*points).end();
                     it2++, point_index++) {
                    sf::Vector2<double> *p = (*points)[point_index];
                    
                    f_points.push_back({ {"x", p->x}, {"y", p->y},{"height",mapgen->getHeight(p->x,p->y)}});
                }

                river_json["points"] = f_points;

                auto f_regions = json::array();

                for (int j = 0; j < r->regions.size(); ++j)
                {
                    f_regions.push_back(j);
                }
                river_json["regions"] = f_regions;

                rivers.push_back(river_json);
            }
            
            n = 0;
            for (auto c : mapgen->map->clusters) {
                auto mc = json({});
                mc["id"] = n;
                mc["name"] = c->name;
                mc["biom"] = c->regions.front()->biom.name;
                mc["regions"] = c->regions.size();
                clusters.push_back(mc);
                n++;
            }
    
            n = 0;
            for (auto c : mapgen->map->megaClusters) {
                auto mc = json({});
                mc["id"] = n;
                mc["name"] = c->name;
                mc["clusters"] = json::array();
                for (auto r : c->regions)
                {
                    auto it = std::find(mapgen->map->clusters.begin(), mapgen->map->clusters.end(), r->cluster);
                    mc["clusters"].push_back(std::distance(mapgen->map->clusters.begin(), it));
                }
    
                auto v = mc["clusters"];
                std::sort(v.begin(), v.end());
                auto last = std::unique(v.begin(), v.end());
                v.erase(last, v.end());
                mc["clusters"] = v;
                mc["regions"] = c->regions.size();
                mClusters.push_back(mc);
                n++;
            }
    
    
            int i = 0;
            for (auto r : mapgen->map->regions) {
                auto json_r = json({});
                auto f_points = json::array();
                auto f_neighbors = json::array();
                auto points = r->getPoints();
                
                int n = 0;
                for (PointList::iterator it2 = points.begin(); it2 < points.end();
                    it2++, n++) {
                    sf::Vector2<double> *p = points[n];
                    f_points.push_back({ {"x", p->x}, {"y", p->y}, {"height", r->getHeight(p)} });
                }
                for (int k = 0; k < r->neighbors.size(); ++k) 
                {
                    f_neighbors.push_back(k);
                }
                
                
                json_r["points"] = f_points;
    
                json_r["site"] = { {"x", r->site->x}, {"y", r->site->y}, {"height", r->getHeight(r->site)} };
                json_r["biom"] = r->biom.name;
                json_r["isLand"] = r->megaCluster->isLand;
                json_r["isBorder"] = r->border;
                json_r["hasRiver"] = r->hasRiver;
                json_r["hasRoad"] = r->hasRoad;
                json_r["humidity"] = r->humidity;
                json_r["nice"] = r->nice;
                json_r["seaBorder"] = r->seaBorder;
                json_r["stateBorder"] = r->stateBorder;
                json_r["temperature"] = r->temperature;
                json_r["traffic"] = r->traffic;
                json_r["windForce"] = r->windForce;
                 
                
                auto it = std::find(mapgen->map->megaClusters.begin(), mapgen->map->megaClusters.end(), r->megaCluster);
                json_r["megaCluster"] = std::distance(mapgen->map->megaClusters.begin(), it);
    
                it = std::find(mapgen->map->clusters.begin(), mapgen->map->clusters.end(), r->cluster);
                json_r["cluster"] = std::distance(mapgen->map->clusters.begin(), it);
                json_r["neighbors"] = f_neighbors;
                regions.push_back(json_r);
                i++;
            }
            
    
            world["regions"] = regions;
            world["megaClusters"] = mClusters;
            world["clusters"] = clusters;
            world["rivers"] = rivers;
            auto dump = world.dump();
            std::strcpy(buff, dump.c_str());
            return dump.length() + 1;
  }
  
  
    UNITY_LIB_API void getRegion(char *str, int n)
	{
        strncpy(str, buff, n);
	}
    
    
    UNITY_LIB_API float GetHeightMapNoise(int x, int y)
    {
       return _heightMap.GetValue(x,y);        
    }
        
    UNITY_LIB_API float InitHeightMapNoise(int w,int h,int octaves,float freq,int seed,const char* terrainT)
    {
        _terrainType = std::string (terrainT);    
        _w = w;
        _h = h;
        _octaves = octaves;
        _freq = freq;
        _seed = seed;
        
        utils::NoiseMapBuilderPlane heightMapBuilder;
        heightMapBuilder.SetDestNoiseMap(_heightMap);

        _perlin.SetSeed(_seed);
        _perlin.SetOctaveCount(_octaves);
        _perlin.SetFrequency(_freq);

        module::Billow terrainType;
        module::RidgedMulti mountainTerrain;
        module::Select finalTerrain;
        module::ScaleBias flatTerrain;
        module::Turbulence tModule;

        if (_terrainType == "archipelago") {

            terrainType.SetFrequency(0.5);
            terrainType.SetPersistence(0.5);

            terrainType.SetSeed(_seed + 1);
            mountainTerrain.SetSeed(_seed + 2);

            finalTerrain.SetSourceModule(0, _perlin);
            finalTerrain.SetSourceModule(1, mountainTerrain);
            finalTerrain.SetControlModule(terrainType);
            finalTerrain.SetBounds(0.0, 100.0);
            finalTerrain.SetEdgeFalloff(0.125);
            heightMapBuilder.SetSourceModule(terrainType);

        } else if (_terrainType == "new") {
            terrainType.SetFrequency(0.3);
            terrainType.SetPersistence(0.4);

            terrainType.SetSeed(_seed);
            mountainTerrain.SetSeed(_seed);

            flatTerrain.SetSourceModule(0, _perlin);
            flatTerrain.SetScale(0.3);
            flatTerrain.SetBias(0.1);

            mountainTerrain.SetFrequency(0.9);

            tModule.SetFrequency(1.5);
            tModule.SetPower(1.5);

            finalTerrain.SetSourceModule(0, tModule);
            finalTerrain.SetSourceModule(1, flatTerrain);
            finalTerrain.SetSourceModule(2, mountainTerrain);
            finalTerrain.SetControlModule(terrainType);
            finalTerrain.SetBounds(0.0, 100.0);
            finalTerrain.SetEdgeFalloff(0.125);
            heightMapBuilder.SetSourceModule(terrainType);

        } else {
            heightMapBuilder.SetSourceModule(_perlin);
        }

        heightMapBuilder.SetDestSize(_w, _h);
        heightMapBuilder.SetBounds(0.0, 10.0, 0.0, 10.0);
        heightMapBuilder.Build();
    }
    
    UNITY_LIB_API float ClearHeightMap()
    {
            
    }
    
}
