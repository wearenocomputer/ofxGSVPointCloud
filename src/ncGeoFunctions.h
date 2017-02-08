//http://barankahyaoglu.com/dev/pokemongo-clone-using-mapzen-api-unity3d/

#pragma once

#include "ofMain.h"
#include "ofxXmlSettings.h"
#include <math.h>

#ifndef TILESIZE
#define TILESIZE  256
#endif

#ifndef R_EARTH
#define R_EARTH 6378137
#endif

#ifndef INITIALRESOLUTION
#define INITIALRESOLUTION (2 * M_PI * R_EARTH / TILESIZE)
#endif

#ifndef ORIGINSHIFT
#define ORIGINSHIFT (2 * M_PI * R_EARTH / 2)
#endif

//Resolution (meters/pixel) for given zoom level (measured at Equator)
inline double ncResolution(int zoom) {
    return INITIALRESOLUTION / (pow(2, zoom));
}

//Converts given lat/lon in WGS84 Datum to XY in Spherical Mercator EPSG:900913
inline ofVec2f ncLatLonToMeters(double lat, double lon) {
    ofVec2f toreturn;
    toreturn.x = (float)(lon * ORIGINSHIFT / 180);
    toreturn.y = (float)(log(tan((90 + lat) * M_PI / 360)) / (M_PI / 180));
    toreturn.y = (float)(toreturn.y * ORIGINSHIFT / 180);
    return toreturn;
}

//Converts pixel coordinates in given zoom level of pyramid to EPSG:900913
inline ofVec2f ncPixelsToMeters(ofVec2f p, int zoom) {
    double res = ncResolution(zoom);
    ofVec2f toreturn;
    toreturn.x = (float)(p.x * res - ORIGINSHIFT);
    toreturn.y = -(float)(p.y * res - ORIGINSHIFT);
    return toreturn;
}

//Converts EPSG:900913 to pyramid pixel coordinates in given zoom level
inline ofVec2f ncMetersToPixels(ofVec2f m, int zoom) {
    double res = ncResolution(zoom);
    ofVec2f toreturn;
    toreturn.x = (float)((m.x + ORIGINSHIFT) / res);
    toreturn.y = (float)((-m.y + ORIGINSHIFT) / res);
    return toreturn;
}


//Returns a TMS (NOT Google!) tile covering region in given pixel coordinates
inline ofVec2f ncPixelsToTile(ofVec2f p) {
    ofVec2f toreturn;
    toreturn.x = (int)ceil(p.x / (double)TILESIZE) - 1;
    toreturn.y = (int)ceil(p.y / (double)TILESIZE) - 1;
    return toreturn;
}


//Returns tile for given mercator coordinates
inline ofVec2f ncMetersToTile(ofVec2f m, int zoom) {
    ofVec2f p = ncMetersToPixels(m, zoom);
    return ncPixelsToTile(p);
}

//Returns bounds of the given tile in EPSG:900913 coordinates
inline ofRectangle ncTileBounds(ofVec2f t, int zoom) {
    ofVec2f min = ncPixelsToMeters(ofVec2f(t.x * TILESIZE, t.y * TILESIZE), zoom);
    ofVec2f max = ncPixelsToMeters(ofVec2f((t.x + 1) * TILESIZE, (t.y + 1) * TILESIZE), zoom);
    return ofRectangle(min.x, min.y, max.x - min.x, max.y - min.y);
}

inline int ncLong2tilex(double lon, int z){
    return (int)(std::floor((lon + 180.0) / 360.0 * std::pow(2.0, z)));
}

inline int ncLat2tiley(double lat, int z){
    return (int)(std::floor((1.0 - std::log( std::tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * std::pow(2.0, z)));
}


inline string url_encode(const string &value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;
    
    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);
        
        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        
        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char)c);
        escaped << nouppercase;
    }
    
    return escaped.str();
}

//convert google polyline code by Paulo Barcelos
//https://github.com/paulobarcelos
inline vector<ofVec2f> decode(string encoded) {
    
    int len = encoded.length();
    int index = 0;
    vector<ofVec2f> points;
    float lat = 0;
    float lng = 0;
    
    while (index < len) {
        char b;
        int shift = 0;
        int result = 0;
        do {
            b = encoded.at(index++) - 63;
            result |= (b & 0x1f) << shift;
            shift += 5;
        } while (b >= 0x20);
        float dlat = ((result & 1) ? ~(result >> 1) : (result >> 1));
        lat += dlat;
        
        shift = 0;
        result = 0;
        do {
            b = encoded.at(index++) - 63;
            result |= (b & 0x1f) << shift;
            shift += 5;
        } while (b >= 0x20);
        float dlng = ((result & 1) ? ~(result >> 1) : (result >> 1));
        lng += dlng;
        
        ofVec2f point;
        point.y = lat * (float)1e-5;
        point.x = lng * (float)1e-5;
        points.push_back(point);
    }
    
    return points;
}

//GOOGLE GEO STUFF
inline vector<float> GoogleAddressToLatLon(string vars){
   
    string encodedvars = url_encode(vars);
    ofHttpRequest req = ofHttpRequest("https://maps.googleapis.com/maps/api/geocode/xml?address=" + encodedvars, "googlegeo");
    ofHttpResponse resp = ofLoadURL(req.url);
    
    ofxXmlSettings data;
    data.loadFromBuffer(resp.data);
    
    string lat = data.getValue("GeocodeResponse:result:geometry:location:lat", "");
    string lon = data.getValue("GeocodeResponse:result:geometry:location:lng", "");
    
    vector<float> tosend;
    tosend.push_back(ofToFloat(lat));
    tosend.push_back(ofToFloat(lon));
    
    return tosend;

}

inline vector<ofVec2f> GoogleDirectionsConvertor(string _origin, string _destination) {
    
    string encodedorigin = url_encode(_origin);
    string encodeddestination = url_encode(_destination);
    
    ofHttpRequest req = ofHttpRequest("https://maps.googleapis.com/maps/api/directions/xml?origin=" + encodedorigin+"&destination="+encodeddestination, "googledirections");
    ofHttpResponse resp = ofLoadURL(req.url);
    
    ofxXmlSettings data;
    data.loadFromBuffer(resp.data);
    
    vector<ofVec2f> points = decode(data.getValue("DirectionsResponse:route:overview_polyline:points", ""));
    
    return points;
    
}






