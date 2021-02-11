/* 
* CIS 2750 - Software Systems Development and Integration
* Name : Zeynep Erdogru
* Student Number : 1047085
* Email : zerdogru@uoguelph.ca
*/
//valgrind --leak-check=full  --show-leak-kinds=all  --track-origins=yes --verbose
#include "../include/GPXParser.h"
#include "../include/LinkedListAPI.h"
#include <ctype.h>

/* Macro */
#define NULLCHECK(ptr) if(ptr==NULL){return NULL;}

/* function declarations */
void createStruct(xmlNode *a_node, GPXdoc **thedoc, List **waypointList, List **routeList, List **trackList);
xmlNode *readFile(char* fileName, xmlDoc **doc);
void initGPXattributes(xmlNode *cur_node, GPXdoc **thedoc);
void getMyNamespace(xmlNode *a_node, GPXdoc **thedoc);

Waypoint *parseWpt(xmlNode *r_node);
void getWptAttributes(xmlNode *myNode, Waypoint **myWpt);
void getWptData(xmlNode *tmp, Waypoint **myWpt, List **waypointDataList);

Route *parseRoute(xmlNode *r_node);
void getRteData(xmlNode *tmp, Route **myRte, List **wptList, List **routeOtherDataList);

Track *parseTrack(xmlNode *r_node);
void getTrackData(xmlNode *tmp, Track **myTrack, List **trackSegmentsList, List **trackOtherDataList);

TrackSegment * parseTrackSegment(xmlNode *r_node);
void getTrackSegmentData(xmlNode *tmp, TrackSegment **mySegment, List **wptList);

List* getWaypoints(const GPXdoc* doc);
List* getAllRoutes(const GPXdoc* doc);
List* getAllTracks(const GPXdoc* doc);
int numData(List *list, char* type);

int makeSureFileName( char* fileName );

/* Function to create an GPX object based on the contents of an GPX file. */
GPXdoc* createGPXdoc(char* fileName) {

    xmlDoc *doc = NULL;
    xmlNode *root = NULL;

    root = readFile(fileName, &doc);
    if (root == NULL) {
        if (doc != NULL) xmlFreeDoc(doc);
        return NULL;
    }

    GPXdoc *theGPX = (GPXdoc *)malloc(sizeof(GPXdoc));

    List *waypointList = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
    List *routeList = initializeList(&routeToString, &deleteRoute, &compareRoutes);
    List *trackList = initializeList(&trackToString, &deleteTrack, &compareTracks);
    
    createStruct(root, &theGPX, &waypointList, &routeList, &trackList);

    if(strcmp(theGPX->namespace,"") == 0) {
        deleteGPXdoc(theGPX);
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }   

    xmlFreeDoc(doc);
    xmlCleanupParser();
    return theGPX;
}

/* Recursive function to fill the structs */
void createStruct(xmlNode *a_node, GPXdoc **thedoc, List **waypointList, List **routeList, List **trackList) {
    xmlNode *cur_node = NULL;
    for (cur_node = a_node; cur_node != NULL; cur_node = cur_node->next) {
       
        /* Calling getNamespace to get the namespace from the xml object */
        getMyNamespace(cur_node, thedoc);

        if (strcmp((char *)cur_node->name, "gpx") == 0) {
            initGPXattributes(cur_node, thedoc);
        }

        if (strcmp((char *)cur_node->name, "wpt") == 0)
        {
            Waypoint *wpt = parseWpt(cur_node);
            if (wpt != NULL) {
                insertBack((*waypointList), (void *)wpt);
            }
        }
        if (strcmp((char *)cur_node->name, "rte") == 0) {
            Route *rte = parseRoute(cur_node);
            if (rte != NULL) {
                insertBack((*routeList), (void *)rte);
            }
        }
        if (strcmp((char *)cur_node->name, "trk") == 0) {
            Track *trackk = parseTrack(cur_node);
            if (trackk != NULL) {
                insertBack((*trackList), (void *)trackk);
            }
        }

        (*thedoc)->tracks = (*trackList);
        (*thedoc)->waypoints = (*waypointList);
        (*thedoc)->routes = (*routeList);

        /* Recurse */
        createStruct(cur_node->children, thedoc, waypointList, routeList, trackList);
    }
}

/* Helper function to get the namespace from the xml object */
void getMyNamespace(xmlNode *a_node, GPXdoc **thedoc) {

    if (a_node->ns != NULL) {
        xmlNs *myNameSpace;
        if ((myNameSpace = a_node->ns) != NULL) {
            strncpy((*thedoc)->namespace, (char *)myNameSpace->href, 255);
            (*thedoc)->namespace[255] = '\0';
        } else {
            /*If there is no namespace, cause error*/
            strcpy((*thedoc)->namespace,"");
        }
    }
}

/* Helper function to parse Track */
Track *parseTrack(xmlNode *r_node) {
    NULLCHECK(r_node)

    Track *myTrack = (Track *)malloc(sizeof(Track));
    NULLCHECK(myTrack)

    /* Everything must be initialized */
    myTrack->name = "";
    List *trackSegmentsList = initializeList(&trackSegmentToString, &deleteTrackSegment, &compareTrackSegments);
    List *trackOtherDataList = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

    if (r_node->children == NULL) {
        /* If rte doesn't have a child node */
        myTrack->otherData = trackOtherDataList;
        myTrack->segments = trackSegmentsList;
        return myTrack;
    }
    xmlNode *temp = r_node->children;
    while ((temp->type != XML_ELEMENT_NODE) && (temp->next != NULL) ) {
        temp = temp->next;
    }

    getTrackData(temp, &myTrack,&trackSegmentsList, &trackOtherDataList);

    myTrack->otherData = trackOtherDataList;
    myTrack->segments = trackSegmentsList;

    return myTrack;
}

/* Helper function to parse route name, waypoints and any other data */
void getTrackData(xmlNode *tmp, Track **myTrack, List **trackSegmentsList, List **trackOtherDataList) {
    xmlNode * tmpNode = NULL;

    for (tmpNode = tmp; tmpNode != NULL; tmpNode = tmpNode->next) {

        if (tmpNode->type == XML_ELEMENT_NODE) {

            char *nodeName = (char *)tmpNode->name;

            if (strcmp(nodeName, "name") == 0) {
                xmlNode *childTemp = tmpNode->children;
                char *cont = (char *)(childTemp->content);

                if (cont != NULL) {
                    (*myTrack)->name = malloc(sizeof(char) * (strlen(cont) + 1));
                    strcpy((*myTrack)->name, cont);
                }
            }
            else if ((strcmp(nodeName, "trkseg") == 0)) {
                TrackSegment *trackSegment = NULL;
                trackSegment = parseTrackSegment(tmpNode);
                if (trackSegment != NULL) {
                    insertBack((*trackSegmentsList), (void *)trackSegment);
                }
            }
            else {
                xmlNode *childTemp = tmpNode->children;
                char *cont = (char *)(childTemp->content);

                if (cont != NULL) {

                    GPXData * newData = malloc(sizeof(GPXData) +255);

                    strcpy(newData->name, nodeName);
                    strcpy(newData->value, cont);
                    strcat(newData->value, "\0");
                    
                    insertBack((*trackOtherDataList), (void *)newData);
                }
            }
        }
    }
}

/* Helper function to parse Track Segments */
TrackSegment * parseTrackSegment(xmlNode *r_node) {

    NULLCHECK(r_node)
    TrackSegment *mySegment = malloc(sizeof(TrackSegment));
    NULLCHECK(mySegment)

    /* Everything must be initialized */
    List *wptList = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
    if (r_node->children == NULL) { 
        /* If rte doesn't have a child node */
        mySegment->waypoints = wptList;
        return mySegment;
    }
    xmlNode *temp = r_node->children;
    while ((temp->type != XML_ELEMENT_NODE) && (temp->next != NULL) ) {
        temp = temp->next;
    }

    getTrackSegmentData(temp, &mySegment, &wptList);

    mySegment->waypoints = wptList;

    return mySegment;
}

/* Helper function to parse waypoints inside tracksegment */
void getTrackSegmentData(xmlNode *tmp, TrackSegment **mySegment, List **wptList) {

    xmlNode * tmpNode = NULL;
    for (tmpNode = tmp; tmpNode != NULL; tmpNode = tmpNode->next) {

        if (tmpNode->type == XML_ELEMENT_NODE) {

            char *nodeName = (char *)tmpNode->name;
            if (strcmp(nodeName, "trkpt") == 0) {

                Waypoint *wpt = parseWpt(tmpNode);
                if (wpt != NULL) {
                    insertBack((*wptList), (void *)wpt);
                }
            }
        }
    }
}

/* Helper function to parse route */
Route *parseRoute(xmlNode *r_node) {
    NULLCHECK(r_node)

    Route *myRte = (Route *)malloc(sizeof(Route));
    NULLCHECK(myRte)

    /* Everything must be initialized */
    myRte->name = "";
    List *wptList = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
    List *routeOtherDataList = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

    if (r_node->children == NULL) { 
        /* If rte doesn't have a child node */
        myRte->otherData = routeOtherDataList;
        myRte->waypoints = wptList;
        return myRte;
    }
    xmlNode *temp = r_node->children;
    while ((temp->type != XML_ELEMENT_NODE) && (temp->next != NULL) ) {
        temp = temp->next;
    }

    getRteData(temp, &myRte,&wptList, &routeOtherDataList);

    myRte->otherData = routeOtherDataList;
    myRte->waypoints = wptList;

    return myRte;
}

/* Helper function to parse route name, waypoints and any other data */
void getRteData(xmlNode *tmp, Route **myRte, List **wptList, List **routeOtherDataList) {

    xmlNode * tmpNode = NULL;
    for (tmpNode = tmp; tmpNode != NULL; tmpNode = tmpNode->next) {

        if (tmpNode->type == XML_ELEMENT_NODE) {

            char *nodeName = (char *)tmpNode->name;

            if (strcmp(nodeName, "rtept") == 0) {
                Waypoint *wpt = NULL;
                wpt = parseWpt(tmpNode);
                
                if (wpt != NULL) {
                    insertBack((*wptList), (void *)wpt);
                }
            }
            else if (strcmp(nodeName, "name") == 0) {
                if ((tmpNode->children->content) != NULL) {

                    xmlNode *childTemp = tmpNode->children;
                    (*myRte)->name = malloc(sizeof(char) * (strlen((char *)(childTemp->content)) + 1));
                    strcpy((*myRte)->name, (char *)(childTemp->content));
                }
            }
            else {
                if ((tmpNode->children->content) != NULL) {

                    xmlNode *childTemp = tmpNode->children;
                    char *cont = (char *)(childTemp->content);

                    GPXData * newData = malloc(sizeof(GPXData) + 255);

                    strcpy(newData->name, nodeName);
                    strcpy(newData->value, cont);
                    strcat(newData->value, "\0");

                    insertBack((*routeOtherDataList), (void *)newData);
                }
            }
        }
    }
}

/* Helper function to parse wpt */
Waypoint *parseWpt(xmlNode *r_node) {

    NULLCHECK(r_node)
    Waypoint *myWpt = NULL;
    myWpt = (Waypoint *)malloc(sizeof(Waypoint));
    NULLCHECK(myWpt)

    myWpt->name = "";

    xmlNode *myNode = r_node;

    List *wptOtherDataList = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);
 
    /* Get waypoint attributes and data */
    getWptAttributes(myNode, &myWpt);

    /* check if wpt has a child node */
    if (r_node->children == NULL) { 
        /* If wpt doesn't have a child node */
        myWpt->otherData = wptOtherDataList;
        return myWpt;
    }

    xmlNode *temp = r_node->children;
    while ((temp->type != XML_ELEMENT_NODE) && (temp->next != NULL) ) {
        temp = temp->next;
    }

    getWptData(temp, &myWpt, &wptOtherDataList);
    myWpt->otherData = wptOtherDataList;

    return myWpt;
}

/* Helper. Parse the waypoint attributes into the waypoint struct. */
void getWptAttributes(xmlNode *myNode, Waypoint **myWpt) {

    xmlAttr *attr = NULL;
    for (attr = myNode->properties; attr != NULL; attr = attr->next) {
        char *attrName = (char *)attr->name;
        xmlNode *value = attr->children;
        char *cont = (char *)(value->content);

        if (strcmp(attrName, "lat") == 0) {
            double lat_tobe = strtod(cont,NULL);
            (*myWpt)->latitude = lat_tobe;
        }
        if (strcmp(attrName, "lon") == 0) {
            double lon_tobe = strtod(cont,NULL);
            (*myWpt)->longitude = lon_tobe;
        }
    }
}

/* Helper function to parse waypoint name and any other data */
void getWptData(xmlNode *tmp, Waypoint **myWpt, List **waypointDataList) {

    xmlNode * tmpNode = NULL;

    for (tmpNode = tmp; tmpNode != NULL; tmpNode = tmpNode->next) {

        if (tmpNode->type == XML_ELEMENT_NODE) {

            char *nodeName = (char *)tmp->name;
            xmlNode *childTemp = tmp->children;

            if (strcmp(nodeName, "name") == 0) {
                char *cont = (char *)(childTemp->content);
                if (cont != NULL) {
                    (*myWpt)->name = malloc(sizeof(char)* (strlen(cont) + 1));
                    strcpy((*myWpt)->name, cont);
                }
            }
            else {
                char *cont = (char *)(childTemp->content);
                if (cont != NULL) {

                    GPXData * newData = malloc(sizeof(GPXData) + 255);

                    strcpy(newData->name, nodeName);
                    strcpy(newData->value, cont);
                    insertBack((*waypointDataList), (void *)newData);
                }
            }
        }
    } 
}

/* Helper function to initialize gpx attributes */
void initGPXattributes(xmlNode *cur_node, GPXdoc **thedoc) {
    xmlNode *the_node = cur_node;
    xmlAttr *attr;
    double version_tobe;

    for (attr = the_node->properties; attr != NULL; attr = attr->next) {

        char *attrName = (char *)attr->name;
        xmlNode *value = attr->children;
        char *cont = (char *)(value->content);

        if (strcmp(attrName, "version") == 0) {
            version_tobe = strtod(cont,NULL);
            (*thedoc)->version = version_tobe;
        }

        if (strcmp(attrName, "creator") == 0) {
            (*thedoc)->creator = malloc(sizeof(char) * (strlen(cont) + 1));
            strcpy((*thedoc)->creator, cont);
        }
    }
}

/* Helper function to open and read xml file */
xmlNode *readFile(char* fileName, xmlDoc **doc) {

    xmlNode *root = NULL;
    //Macro to check that the libxml version in use
    LIBXML_TEST_VERSION

    //parse the file and get the DOM 
    *doc = xmlReadFile(fileName, NULL, 0);
    if((*doc) == NULL) {
        xmlFreeDoc(*doc);
        xmlCleanupParser();
        return NULL;
    }

    if(makeSureFileName(fileName) == -1) { 
        free(fileName);
        return NULL;
    }

    /* parse the file */
    root = xmlDocGetRootElement(*doc);
    if(root == NULL) {
        xmlFreeDoc(*doc);
        xmlCleanupParser();
        return NULL;
    }
    return root;
}

int makeSureFileName( char* fileName ) {

    if (fileName == NULL) {
        return -1;
    } else if (strcmp(fileName,"") == 0) {
        return -1;
    } 
    if(strstr(fileName, ".gpx") != NULL) return 0;
    return -1;
}

/* Function to create a string representation of an GPX object. */
char* GPXdocToString(GPXdoc* doc) {
    char* description;
    char* temp;
    int size = 0;

    if(doc == NULL) {
        return NULL;
    }

    /* Find length of everything before malloc */
    size += strlen("Namespace: %s,\nVersion: %f");
    size += strlen(doc->namespace);
    size += sizeof(double) * 1;

    if(strcmp(doc->creator,"") != 0) { 
        size += strlen("\nCreator: ");
        size += strlen(doc->creator);
    }

    if(getLength(doc->waypoints) > 0) {
        size += strlen("\n\nWaypoints: ");
        temp = toString(doc->waypoints);
        size += strlen(temp);
        free(temp);
    }

    if(getLength(doc->routes) > 0) {
        temp = toString(doc->routes);
        size += strlen(temp);
        free(temp);
    }

    if(getLength(doc->tracks) > 0) {
        temp = toString(doc->tracks);
        size += strlen(temp);
        free(temp);
    }

    description = malloc(sizeof(char)*size + 1);

    /*Copy everything in*/
    sprintf(description,"Namespace: %s,\nVersion: %f", doc->namespace, doc->version);

    if(strcmp(doc->creator,"") != 0)  {
        strcat(description,"\nCreator: ");
        strcat(description,(doc->creator));
    }

    if(getLength(doc->waypoints) > 0) {
        strcat(description,"\n\nWaypoints: ");
        temp = toString(doc->waypoints);
        strcat(description,temp);
        free(temp);
    }

    if(getLength(doc->routes) > 0) {
        temp = toString(doc->routes);
        strcat(description,temp);
        free(temp);
    }

    if(getLength(doc->tracks) > 0) {
        temp = toString(doc->tracks);
        strcat(description,temp);
        free(temp);
    }
    
    description[size] = '\0';
    return description;
}

/* Function to delete doc content and free all the memory. */
void deleteGPXdoc(GPXdoc* doc) {

    if(doc == NULL) {
        return;
    }

    if (doc->creator != NULL) {
        free(doc->creator);
    }

    if (doc->waypoints != NULL) {
        freeList(doc->waypoints);
    }

    if (doc->routes != NULL) {
        freeList(doc->routes);
    }

    if (doc->tracks != NULL) {
        freeList(doc->tracks);
    }
    free(doc);
}


/* ******************************* List helper functions  - MUST be implemented *************************** */

/* GPXData helper functions */
void deleteGpxData(void* data) {
    GPXData *temp = NULL;

    if (data == NULL) {
        return;
    }
    temp = (GPXData *)data;
    free(temp);
}

char* gpxDataToString(void* data) {
    char *myString;
    int length = 0;

    GPXData *temp = (GPXData *)data;
    NULLCHECK(temp)

    //get the length for malloc
    length = strlen(temp->name) + strlen(temp->value) + strlen("\t: ");
    myString = malloc(sizeof(char) * (length+1));

    //strcpy info in the string 
    strcpy(myString,"\t");
    strcat(myString,temp->name);
    strcat(myString,": ");
    strcat(myString,temp->value);
    myString[length] = '\0';
    
    return myString;
}

int compareGpxData(const void *first, const void *second) {
    return 0;
}

/* Waypoint helper functions */
void deleteWaypoint(void* data) {
    if (data == NULL) {
        return;
    }

    Waypoint *temp_wpt = (Waypoint *)data;
    temp_wpt->latitude = 0.0;
    temp_wpt->longitude = 0.0;

    if((temp_wpt->name != NULL)&& strcmp(temp_wpt->name, "") != 0 ) {
        free(temp_wpt->name);
    }

    freeList(temp_wpt->otherData);
    free(temp_wpt);
}

char* waypointToString( void* data) {
    int len = 0;                                   
    char *myWptString;
    char *temp;
    Waypoint *temp_wpt = data;
    NULLCHECK(temp_wpt)

    /*Get size before malloc*/
    len += strlen("Name: %s, Longitude: %lf, Latitude: %lf");
    len += strlen(temp_wpt->name);
    len += sizeof(double) * 2;
    
    if(getLength(temp_wpt->otherData) > 0){
        len += strlen("\n\tOther: ");
        temp = toString(temp_wpt->otherData);
        len += strlen(temp);
        free(temp);
    }

    myWptString = malloc(sizeof(char)*len + 1); 

    /*Copy it all in*/
    sprintf(myWptString,"Name: %s, Longitude: %lf, Latitude: %lf", temp_wpt->name, temp_wpt->longitude,temp_wpt->latitude);
  
    if(getLength(temp_wpt->otherData) > 0) {
        strcat(myWptString,"\n\tOther: ");
        temp = toString(temp_wpt->otherData);
        strcat(myWptString,temp);
        free(temp);
    }
    myWptString[len] = '\0';
    return myWptString;
}

int compareWaypoints(const void *first, const void *second) {
    return 0;
}

/* Route helper functions */
void deleteRoute(void* data) {
    if (data == NULL) return;

    Route *temp_route = (Route *)data;
    
    if((temp_route->name != NULL) && strcmp(temp_route->name, "") != 0 ) {
        free(temp_route->name);
    }
    freeList(temp_route->waypoints);
    freeList(temp_route->otherData);

    free(temp_route);
}

char* routeToString(void* data) {
    int size = 0;                                   
    char *string;
    char *temp;
    Route *temp_route = data;
    NULLCHECK(temp_route)

    /*Get size before malloc*/
    size += strlen("\nRoute: ");
    size += strlen(temp_route->name);
    temp = toString(temp_route->waypoints);
    size += strlen(temp);
    free(temp);

    if(getLength(temp_route->otherData) > 0){
        size += strlen("\nOther: ");
        temp = toString(temp_route->otherData);
        size += strlen(temp);
        free(temp);
    }

    string = malloc(sizeof(char)*size + 1);

    /*Copy it all in*/
    strcpy(string,"\nRoute: ");
    strcat(string,temp_route->name);
    temp = toString(temp_route->waypoints);
    strcat(string,temp);
    free(temp);
    
    if(getLength(temp_route->otherData) > 0) {
        strcat(string,"\nOther: ");
        temp = toString(temp_route->otherData);
        strcat(string,temp);
        free(temp);
    }
    string[size] = '\0';
    return string;
}

int compareRoutes(const void *first, const void *second) {
    return 0;
}

/* TrackSegment helper functions */
void deleteTrackSegment(void* data) {
    if (data == NULL) return;

    TrackSegment *temp_trackseg = (TrackSegment *)data;

    freeList(temp_trackseg->waypoints);
    free(temp_trackseg);
}

char* trackSegmentToString(void* data)  {
    TrackSegment *temp_trackseg = data;
    NULLCHECK(temp_trackseg)
    int size = 0;                                   
    char *string;
    char *temp;

    /*Get size before malloc*/
    size += strlen("\nTrack Segment: ");
    temp = toString(temp_trackseg->waypoints);
    size += strlen(temp);
    free(temp);

    string = malloc(sizeof(char)*size + 1);
    
    /*Copy it all in*/
    strcpy(string,"\nTrack Segment: ");
    temp = toString(temp_trackseg->waypoints);
    strcat(string,temp);
    free(temp);

    string[size] = '\0';

    return string;
}

int compareTrackSegments(const void *first, const void *second) {
    return 0;
}

/* Track helper functions */
void deleteTrack(void* data) {
    Track *temp_track;
    if (data == NULL) {
        return;
    }
    temp_track = (Track *)data;

    if((temp_track->name != NULL)&& strcmp(temp_track->name, "") != 0 ) {
        free(temp_track->name);
    }

    freeList(temp_track->segments);
    freeList(temp_track->otherData);

    free(temp_track);
}

char* trackToString(void* data) {
    int size = 0;                                   
    char *string;
    char *temp;
    Track *temp_track = data;
    NULLCHECK(temp_track)

    /*Get size before malloc*/
    size += strlen("\nTrack: ");
    size += strlen(temp_track->name);
    temp = toString(temp_track->segments);
    size += strlen(temp);
    free(temp);

    if(getLength(temp_track->otherData) > 0){
        size += strlen("\nOther: ");
        temp = toString(temp_track->otherData);
        size += strlen(temp);
        free(temp);
    }

    string = malloc(sizeof(char)*size + 1);

    /*Copy it all in*/
    strcpy(string,"\nTrack: ");
    strcat(string,temp_track->name);
    temp = toString(temp_track->segments);
    strcat(string,temp);
    free(temp);
    
    if(getLength(temp_track->otherData) > 0) {
        strcat(string,"\nOther: ");
        temp = toString(temp_track->otherData);
        strcat(string,temp);
        free(temp);
    }

    string[size] = '\0';

    return string;
}

int compareTracks(const void *first, const void *second) {
    return 0;
}

/* ***************************** MODULE 2 FUNCTIONS ***************************** */

/* Function that returns a waypoint with the given name.  If more than one exists, return the first one. */
Waypoint* getWaypoint(const GPXdoc* doc, char* name) {
    Waypoint *tmpWpt = NULL;
    NULLCHECK(doc)

    void *elem;
    ListIterator iter = createIterator(doc->waypoints);
    while ((elem = nextElement(&iter)) != NULL)
    {
        tmpWpt = (Waypoint *)elem;
        char *name_wpt = tmpWpt->name;
        if (strcmp(name_wpt, name) == 0) {
            return tmpWpt;
        }
    }

    return NULL;
}
/* Function that returns a track with the given name.  If more than one exists, return the first one. */
Track* getTrack(const GPXdoc* doc, char* name) {
    Track *tmptrack = NULL;
    NULLCHECK(doc)

    void *elem;
    ListIterator iter = createIterator(doc->tracks);
    while ((elem = nextElement(&iter)) != NULL)
    {
        tmptrack = (Track *)elem;
        char *name_trk = tmptrack->name;
        if (strcmp(name_trk, name) == 0) {
            return tmptrack;
        }
    }

    return NULL;
}


/* Function that returns a route with the given name.  If more than one exists, return the first one. */
Route* getRoute(const GPXdoc* doc, char* name) {
    Route *tmpRoute = NULL;
    NULLCHECK(doc)

    void *elem;
    ListIterator iter = createIterator(doc->routes);
    while ((elem = nextElement(&iter)) != NULL)
    {
        tmpRoute = (Route *)elem;
        char *name_route = tmpRoute->name;
        if (strcmp(name_route, name) == 0) {
            return tmpRoute;
        }
    }
    return NULL;
}

/* Total number of waypoints in the GPX file */
int getNumWaypoints(const GPXdoc* doc) {
    if (doc == NULL) return -1;
    
    int num = 0;
    List *waypoints = getWaypoints(doc);

    for(int i = 0; i < getLength(waypoints); i++) {
        num++;
    }

    return num;
}

/* Helper. Get all waypoints */
List* getWaypoints(const GPXdoc* doc) {
    NULLCHECK(doc)

    List *list = initializeList(waypointToString,deleteWaypoint,compareWaypoints);
    Waypoint *wpt;
    ListIterator wptsss = createIterator(doc->waypoints);

    /*Iterate*/
    for(int i = 0; i < getLength(doc->waypoints); i++) {
        wpt = (Waypoint*)nextElement(&wptsss);
        insertBack(list,wpt);
    }
    
    return list;
}

/* Helper. Get all routes */
List* getAllRoutes(const GPXdoc* doc) {
    NULLCHECK(doc)

    List *list = initializeList(routeToString,deleteRoute,compareRoutes);
    Route *rte;
    ListIterator routesss = createIterator(doc->routes);

    /*Iterate*/
    for(int i = 0; i < getLength(doc->routes); i++) {
        rte = (Route*)nextElement(&routesss);
        insertBack(list,rte);
    }
    
    return list;
}

/* Total number of routes in the GPX file */
int getNumRoutes(const GPXdoc* doc) {
    if (doc == NULL) return -1;
    int num = 0;
    List *routes = getAllRoutes(doc);

    for(int i = 0; i < getLength(routes); i++) {
        num++;
    }
    return num;
}

/* Total number of tracks in the GPX file */
int getNumTracks(const GPXdoc* doc) {
    if (doc == NULL) return -1;
    int num = 0;
    List *tracks = getAllTracks(doc);

    for(int i = 0; i < getLength(tracks); i++) {
        num++;
    }
    return num;
}

/* Helper. Get all tracks */
List* getAllTracks(const GPXdoc* doc) {
    NULLCHECK(doc)

    List *list = initializeList(trackToString,deleteTrack,compareTracks);
    Track *trck;
    ListIterator trackss = createIterator(doc->tracks);

    /* Iterate */
    for(int i = 0; i < getLength(doc->tracks); i++) {
        trck = (Track*)nextElement(&trackss);
        insertBack(list,trck);
    }
    return list;
}


/* Total number of GPXData elements in the document */
int getNumGPXData(const GPXdoc* doc) {
    if (doc ==NULL) return -1;
    int num =0;
    num = num + numData(getWaypoints(doc),"waypoint");
    num = num + numData(getAllTracks(doc),"track");
    num = num + numData(getAllRoutes(doc),"route");

    return num;
}

int numData(List *list, char* type) {
    int num = 0;
    ListIterator iter = createIterator(list);
    
    /*Use the type to determine which length to get*/
    for(int i = 0; i < getLength(list); i++) {
        if(strcmp(type,"waypoint") == 0) {
            Waypoint* wpt = (Waypoint*)nextElement(&iter);
            num += getLength(wpt->otherData);
        } else if(strcmp(type,"track") == 0) {
            Track* trk = (Track*)nextElement(&iter);
            num += getLength(trk->otherData);
        } else if(strcmp(type,"route") == 0) {
            Route* rte = (Route*)nextElement(&iter);
            num += getLength(rte->otherData);
        }
    }
    return num;
}

int getNumSegments(const GPXdoc* doc) {
    return 0;
}