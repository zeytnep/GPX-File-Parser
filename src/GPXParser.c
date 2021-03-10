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
#include <math.h>

/* Macro */
#define NULLCHECK(ptr) if(ptr==NULL){return NULL;}

/* A1 Helper function declarations */
void createStruct(xmlNode *a_node, GPXdoc **thedoc, List **waypointList, List **routeList, List **trackList);
xmlNode *readFile(char* fileName, xmlDoc **doc);
void initGPXattributes(xmlNode *cur_node, GPXdoc **thedoc);
void getMyNamespace(xmlNode *a_node, GPXdoc **thedoc);
int makeSureFileName( char* fileName );

Waypoint *parseWpt(xmlNode *r_node);
void getWptAttributes(xmlNode *myNode, Waypoint **myWpt);
void getWptData(xmlNode *tmp, Waypoint **myWpt, List **waypointDataList);

Route *parseRoute(xmlNode *r_node);
void getRteData(xmlNode *tmp, Route **myRte, List **wptList, List **routeOtherDataList);

Track *parseTrack(xmlNode *r_node);
void getTrackData(xmlNode *tmp, Track **myTrack, List **trackSegmentsList, List **trackOtherDataList);

TrackSegment * parseTrackSegment(xmlNode *r_node);
void getTrackSegmentData(xmlNode *tmp, List **wptList);

List* getWaypoints(const GPXdoc* doc);
List* getAllRoutes(const GPXdoc* doc);
List* getAllTracks(const GPXdoc* doc);
List * getTrackSegmentList(const GPXdoc* doc);
int numData(List *list, char* type);

/* A2 Helper function declarations */
int validateTree(xmlDoc *the_doc, char *gpxSchemaFile);
void freeMyList(List *list);

int validateOtherData(List *list);
int validateWayPoints(List *list);
int validateRoutes(List *list);
int validateTracks(List *list);
int validateTrackSegments(List *list);
int checkRestrictions(GPXdoc* doc);
int checkDoc(xmlDoc *doc, char* schemaFile);
xmlDoc *GPXtoXMLdoc(GPXdoc* doc);

int setRootElementNodes(GPXdoc *doc, xmlNode *root_node);
int setRouteNodes(List *list, xmlNode *root);
int setWaypointNodes(List *list, xmlNode *root, char *type);
int setTrackNodes(List *list, xmlNode *root);
void setsegmentNodes(List *list, xmlNode *root);
void setOtherDataNodes(List *otherDataList, xmlNode *otherDat);

float CalcDistance(Waypoint * num1, Waypoint * num2);


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

    if (strcmp(theGPX->namespace,"") == 0) {
        deleteGPXdoc(theGPX);
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }
    if (theGPX->creator ==  NULL) {
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

    /* If track doesn't have a child node */
    if (r_node->children == NULL) 
    {
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
                GPXData * newData = malloc(sizeof(GPXData) +255);

                strcpy(newData->name, nodeName);
                strcpy(newData->value, cont);
                strcat(newData->value, "\0");
                    
                insertBack((*trackOtherDataList), (void *)newData);
            }
        }
    }
}

/* Helper function to parse Track Segments */
TrackSegment * parseTrackSegment(xmlNode *r_node) {

    TrackSegment *mySegment = malloc(sizeof(TrackSegment));
    NULLCHECK(mySegment)

    /* Everything must be initialized */
    List *wptList = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);

    /* If segment doesn't have a child node */
    if (r_node->children == NULL) 
    {
        mySegment->waypoints = wptList;
        return mySegment;
    }

    xmlNode *temp = r_node->children;
    while ((temp->type != XML_ELEMENT_NODE) && (temp->next != NULL) ) {
        temp = temp->next;
    }

    getTrackSegmentData(temp, &wptList);

    mySegment->waypoints = wptList;

    return mySegment;
}

/* Helper function to parse waypoints inside tracksegment */
void getTrackSegmentData(xmlNode *tmp, List **wptList) {

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
    myRte->name = "\0";
    List *wptList = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
    List *routeOtherDataList = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

    /* If rte doesn't have a child node */
    if (r_node->children == NULL) 
    {
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
                Waypoint *wpt = parseWpt(tmpNode);
                
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

/* Helper function to parse wpt */
Waypoint *parseWpt(xmlNode *r_node) {
    NULLCHECK(r_node)

    Waypoint *myWpt = myWpt = (Waypoint *)malloc(sizeof(Waypoint));
    NULLCHECK(myWpt)

    myWpt->name = "\0";
    myWpt->latitude = 0.0;
    myWpt->longitude = 0.0;
    List *wptOtherDataList = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

    xmlNode *myNode = r_node;
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
    for (attr = myNode->properties; attr != NULL; attr = attr->next) 
    {
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

            char *nodeName = (char *)tmpNode->name;
            xmlNode *childTemp = tmpNode->children;

            if (strcmp(nodeName, "name") == 0) {
                char *cont = (char *)(childTemp->content);
                if (cont != NULL) {
                    (*myWpt)->name = malloc(sizeof(char)* (strlen(cont) + 1));
                    strcpy((*myWpt)->name, cont);
                }
            }
            else {
                char *cont = (char *)(childTemp->content);
                GPXData * newData = malloc(sizeof(GPXData) + 255);
                
                strcpy(newData->name, nodeName);
                strcpy(newData->value, cont);
                insertBack((*waypointDataList), (void *)newData);
            }
        }
    }
}

/* Helper function to initialize gpx attributes */
void initGPXattributes(xmlNode *cur_node, GPXdoc **thedoc) {
    xmlNode *the_node = cur_node;
    xmlAttr *attr;
    double version_tobe;
    int i=0;    

    for (attr = the_node->properties; attr != NULL; attr = attr->next) {

        char *attrName = (char *)attr->name;
        xmlNode *value = attr->children;
        char *cont = (char *)(value->content);

        if (strcmp(attrName, "version") == 0) {
            version_tobe = strtod(cont,NULL);
            (*thedoc)->version = version_tobe;
            i =1;
        }

        if (strcmp(attrName, "creator") == 0) {
            (*thedoc)->creator = malloc(sizeof(char) * (strlen(cont) + 1));
            strcpy((*thedoc)->creator, cont);
        }
    }

    if (i == 0) { //meaning version is uninitialized 
        (*thedoc)->version = 0.0;
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
    size += strlen("Namespace: %s\nVersion: %f");
    size += strlen(doc->namespace);
    size += sizeof(double) *1;

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
    sprintf(description,"Namespace: %s\nVersion: %f", doc->namespace, doc->version);

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

/* ******************************* A2 functions *************************** */

/** Function to create an GPX object based on the contents of an GPX file.
 * This function must validate the XML tree generated by libxml against a GPX schema file
 * before attempting to traverse the tree and create an GPXdoc struct
 *@pre File name cannot be an empty string or NULL.
       File represented by this name must exist and must be readable.
 *@post Either:
        A valid GPXdoc has been created and its address was returned
		or 
		An error occurred, and NULL was returned
 *@return the pinter to the new struct or NULL
 *@param gpxSchemaFile - the name of a schema file
 *@param fileName - a string containing the name of the GPX file
**/
GPXdoc* createValidGPXdoc(char* fileName, char* gpxSchemaFile) {

    xmlDoc *doc = NULL;

    if (fileName == NULL) return NULL;
    if (strcmp(fileName,"") == 0) return NULL;
    if (gpxSchemaFile == NULL) return NULL;
    
    /* Get the doc */
    doc = xmlReadFile(fileName, NULL, 0); 

    if (doc == NULL) {
        xmlCleanupParser();
        return NULL;
    }

    if (validateTree(doc, gpxSchemaFile) == 0) 
    {
        GPXdoc * valid_gpx = createGPXdoc(fileName);
        NULLCHECK(valid_gpx)
        return valid_gpx;
    }

    return NULL;
}

bool validateGPXDoc(GPXdoc* doc, char* gpxSchemaFile) 
{
    if (doc == NULL) return false;

    // check Restrictions
    int areYouValid = checkRestrictions(doc);
    if (areYouValid == -1) return false;

    xmlDoc *xmllldoc = GPXtoXMLdoc(doc);
    if (xmllldoc == NULL) return false;
    
    if(checkDoc(xmllldoc,gpxSchemaFile) != 0) {
        return false;
    }
 
    return true;
} 

int checkDoc(xmlDoc *doc, char* schemaFile) {
    int ret = 0;
    xmlSchemaValidCtxtPtr ctxt = NULL;
    xmlSchemaParserCtxtPtr parser = NULL;
    xmlSchemaPtr schema = NULL;

    xmlLineNumbersDefault(1);
    /*Set up schema*/
    parser = xmlSchemaNewParserCtxt(schemaFile);
    schema = xmlSchemaParse(parser);
    xmlSchemaFreeParserCtxt(parser);
    if(schema == NULL) {
        xmlSchemaFree(schema);
        xmlFreeDoc(doc);
        xmlSchemaCleanupTypes();
        xmlMemoryDump();
        return -1;
    }

    ctxt = xmlSchemaNewValidCtxt(schema);
    ret = xmlSchemaValidateDoc(ctxt, doc);

    /*Cleanup*/
    xmlSchemaFreeValidCtxt(ctxt);
    xmlSchemaFree(schema);
    xmlFreeDoc(doc);
    xmlSchemaCleanupTypes();
    xmlMemoryDump();
    
    return ret;
}


bool writeGPXdoc(GPXdoc*doc, char*fileName) {
    if (doc == NULL) return false;
    if (fileName == NULL) return false;

    xmlDoc *gpxDoc = GPXtoXMLdoc(doc); //This cretaes the xml doc
    if (gpxDoc == NULL) return false;

    int i = xmlSaveFormatFileEnc(fileName, gpxDoc, "UTF-8", 1);
    xmlFreeDoc(gpxDoc);
    xmlCleanupParser();

    if (i == -1) return false;

    return true;
}



/* ********************** A2 helper functions ********************** */
xmlDoc *GPXtoXMLdoc(GPXdoc* doc) 
{

    if (doc == NULL) return NULL;
    char buffer[258];

    xmlDoc *new_doc = xmlNewDoc(BAD_CAST"1.0");
    if (new_doc == NULL) return NULL;

    xmlNode *root = xmlNewNode(NULL, BAD_CAST "gpx"); //Creates the root node.

    sprintf(buffer, "%1f", doc->version);
    xmlNewProp(root, (xmlChar *)"version", BAD_CAST "1.1");
    buffer[0] = '\0';


    if (doc->creator != NULL)
    {
        xmlNewProp(root, (xmlChar *)"creator", (xmlChar *)doc->creator);
    }

    xmlNsPtr ns =  xmlNewNs(root, (xmlChar*)doc->namespace, NULL);
    xmlDocSetRootElement(new_doc,root);
    xmlSetNs(root,ns);


    setWaypointNodes(doc->waypoints, root, "wpt");
    setRouteNodes(doc->routes, root);
    setTrackNodes(doc->tracks, root);


    return new_doc;
}




int setTrackNodes(List *list, xmlNode *root) {
    if (root == NULL) return -1;
    if (list == NULL) return -1;

    ListIterator iter = createIterator(list);
    xmlNode *track_node = NULL;
    void *elem;

    while ((elem = nextElement(&iter)) != NULL) {
        Track *track = (Track *)elem;

        track_node = xmlNewChild(root, NULL, (xmlChar *)"trk", NULL);
        xmlNewChild(track_node, NULL, (xmlChar *)"name", (xmlChar *)(track->name));

        if (track->otherData != NULL) {
            if (getLength(track->otherData) > 0)
            {
                setOtherDataNodes(track->otherData, track_node);
            }
        }
        if (getLength(track->segments) > 0) {
            setsegmentNodes(track->segments, track_node);
        }   
    }
    return 0;
}

void setsegmentNodes(List *list, xmlNode *root) {

    ListIterator iter = createIterator(list);
    xmlNode *segment_node = NULL;
    void *elem;

    while ((elem = nextElement(&iter)) != NULL) {
        TrackSegment *seg = (TrackSegment *)elem;
        segment_node = xmlNewChild(root, NULL, (xmlChar *)"trkseg", NULL);

        if (seg->waypoints != NULL) {
            if (getLength(seg->waypoints) > 0)
            {
                setWaypointNodes(seg->waypoints, segment_node, "trkpt");
            }
        }
    }
}

int setRouteNodes(List *list, xmlNode *root) {
    if (root == NULL) return -1;
    if (list == NULL) return -1;

    ListIterator iter = createIterator(list);
    xmlNode *route_node = NULL;
    void *elem;

    while ((elem = nextElement(&iter)) != NULL) 
    {
        Route *rte = (Route *)elem;
        route_node = xmlNewChild(root, NULL, (xmlChar *)"rte", NULL);
        if ((strcmp(rte->name,"\0") != 0)) {
            xmlNewChild(route_node, NULL, (xmlChar *)"name", (xmlChar *)(rte->name));
        }
        
        if (rte->otherData != NULL) {
            if (getLength(rte->otherData) > 0)
            {
                setOtherDataNodes(rte->otherData, route_node);
            }
        }

        if (getLength(rte->waypoints) > 0) {
            setWaypointNodes(rte->waypoints, route_node, "rtept");
        }
    }
    return 0;
}


int setWaypointNodes(List *list, xmlNode *root, char *type) {
    if (list == NULL) return -1;
    if (root == NULL) return -1;

    ListIterator iter = createIterator(list);
    xmlNode *wpt_node = NULL;
    void *elem;
    char buffer[256];

    while ((elem = nextElement(&iter)) != NULL) 
    {
        Waypoint *wpt = (Waypoint *)elem;
        wpt_node = xmlNewChild(root, NULL, BAD_CAST type, NULL);

        if ((strcmp(wpt->name,"\0") != 0)) {
            xmlNewChild(wpt_node, NULL, BAD_CAST "name",BAD_CAST wpt->name);
        }

        if (wpt->otherData != NULL) {
            if (getLength(wpt->otherData) > 0)
            {
                setOtherDataNodes(wpt->otherData, wpt_node);
            }
        }

        sprintf(buffer, "%f", wpt->latitude);
        xmlNewProp(wpt_node, BAD_CAST "lat", BAD_CAST buffer);
        buffer[0] = '\0';

        sprintf(buffer, "%f", wpt->longitude);
        xmlNewProp(wpt_node, BAD_CAST "lon", BAD_CAST buffer);
        buffer[0] = '\0';

    }
    return 0;
}

void setOtherDataNodes(List *otherDataList, xmlNode *otherDat) {
    ListIterator iter;
    iter = createIterator(otherDataList);
    void *elem;

    while ((elem = nextElement(&iter)) != NULL)
    {
        GPXData *tmpData = (GPXData *)elem;
        xmlNewChild(otherDat, NULL, (xmlChar *)(tmpData->name), (xmlChar *)(tmpData->value));
    }
}


int checkRestrictions(GPXdoc* doc) 
{
    List *list = NULL;

    if(doc->namespace[0] == '\0') return -1;
    if (doc->creator == NULL) return -1;

    if (doc->waypoints == NULL) return -1;
    if (doc->routes == NULL) return -1;
    if (doc->routes == NULL) return -1;

    //validate waypoints
    list = getWaypoints(doc);
    if (validateWayPoints(list) == -1) return -1;
    freeMyList(list);

    //validate routes
    list = getAllRoutes(doc);
    if (validateRoutes(list) == -1) return -1;
    freeMyList(list);

    //validate tracks
    list = getAllTracks(doc);
    if (validateTracks(list) == -1) return -1;
    freeMyList(list);

    //validate track segments
    //gives seg fault
    list = getTrackSegmentList(doc);
    if (validateTrackSegments(list) == -1) return -1;
    freeMyList(list);

    return 0;
}

//check track seg
int validateTracks(List *list) 
{

    ListIterator iter = createIterator(list);
    for (int i = 0; i < getLength(list); i++) 
    {
        Track *trk = (Track*)nextElement(&iter);
        if (trk->name == NULL) 
        {
            return -1;
        }

        if(validateOtherData(trk->otherData) != 0) {
            return -1; //-1 means false
        }
    }
    return 0;
}
//validate track segments
int validateTrackSegments(List *list) 
{
    ListIterator iter = createIterator(list);
    for (int i = 0; i < getLength(list); i++) 
    {
        TrackSegment *trckseg = (TrackSegment*)nextElement(&iter);
        if(validateWayPoints(trckseg->waypoints) != 0) 
        {
            return -1; //-1 means false
        }

    }
    return 0;

}

int validateRoutes(List *list) 
{

    ListIterator iter = createIterator(list);

    for (int i = 0; i < getLength(list); i++) 
    {
        Route *rte = (Route*)nextElement(&iter);
        if (rte->name == NULL) 
        {
            return -1;
        }
        if (validateWayPoints(rte->waypoints) != 0) 
        {
            return -1;
        }
        if(validateOtherData(rte->otherData) != 0)
        {
            return -1; //-1 means false
        }

    }
    return 0;
}


int validateWayPoints(List *list) 
{

    ListIterator iter = createIterator(list);

    for (int i = 0; i < getLength(list); i++) 
    {
        Waypoint *wpt = (Waypoint*)nextElement(&iter);
        if (wpt->name == NULL) 
        {
            return -1; //-1 means false
        }
        if(validateOtherData(wpt->otherData) != 0)
        {
            return -1; //-1 means false
        }
    }
    return 0; //successful == valid waypoints
}


int validateOtherData(List *list) 
{
    ListIterator iter = createIterator(list);
    
    for(int i = 0; i < getLength(list); i++) 
    {
        GPXData *data = (GPXData*)nextElement(&iter);
        if (strcmp(data->name,"\0") == 0) 
        {
            return -1;
        }
        if(strcmp(data->value,"\0") == 0) 
        {
            return -1;
        } 
    }

    return 0;
}


void freeMyList(List *list) 
{
    Node* tmp;

    if (list == NULL) return;

    if (list->head == NULL && list->tail == NULL)
    {
        free(list);
		return;
	}

    while (list->head != NULL)
    {
		tmp = list->head;
		list->head = list->head->next;
		free(tmp);
	}

    list->head = NULL;
	list->tail = NULL;
	list->length = 0;
    free(list);
    return;	
}



int validateTree(xmlDoc *the_doc, char *gpxSchemaFile) {
    xmlDoc *doc = the_doc;
    if (doc == NULL) return -1;

    xmlSchema *schema = NULL;
    xmlSchemaParserCtxt *schema_ctxt;

    xmlLineNumbersDefault(1);

    /* Creates an xml schema parse context. */
    schema_ctxt = xmlSchemaNewParserCtxt(gpxSchemaFile);
    if (schema_ctxt == NULL) 
    {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return -1;
    }
    /* Set the functions for error handling for validation context. */
    xmlSchemaSetParserErrors(schema_ctxt, (xmlSchemaValidityErrorFunc)fprintf, (xmlSchemaValidityWarningFunc)fprintf, stderr);

    /* Parse the schema and point the internal set the internal schema struct. */
    schema = xmlSchemaParse(schema_ctxt);
    if (schema == NULL)
    {
        xmlSchemaFreeParserCtxt(schema_ctxt);
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return -1;
    }

    /* Free the schema parser context. */
    xmlSchemaFreeParserCtxt(schema_ctxt);

    /* Create a valid context. */
    xmlSchemaValidCtxt *valid_ctxt;
    int ret;
    valid_ctxt = xmlSchemaNewValidCtxt(schema);

    if (valid_ctxt == NULL)
    {
        xmlSchemaFree(schema);
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return -1;
    }
    xmlSchemaSetValidErrors(valid_ctxt, (xmlSchemaValidityErrorFunc)fprintf, (xmlSchemaValidityWarningFunc)fprintf, stderr);

    /* Validates the doc againts the schema structure. */
    ret = xmlSchemaValidateDoc(valid_ctxt, doc);

    xmlSchemaFreeValidCtxt(valid_ctxt);
    xmlSchemaFree(schema);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    xmlMemoryDump();
    
    //returns 0 if valid
    return ret;
}




//module 2 functions

float round10(float len) {
    int ret = (int) len;
    ret = ret /10;
    ret = ret *10;
    return (float)ret;
}

//Function that returns the length of a Route in meters
float getRouteLen(const Route *rt) {

    if (rt == NULL) return 0;

    int len = getLength(rt->waypoints);
    Waypoint * arr[len];

    ListIterator iter = createIterator(rt->waypoints);
    float total =0;
    Waypoint *wpt;
    int arrSize =0;

    while ( (wpt = nextElement(&iter)) != NULL) {
        arr[arrSize] = wpt;
        arrSize++;
    }

    for (int j =0; j< len; j++) 
    {
        if ((j+1)==len) return total;
        total = CalcDistance(arr[j], arr[j+1]);
    }
    return total;

}
//helper
float CalcDistance(Waypoint * num1, Waypoint * num2) {
    double phi1 = (num1->latitude) * (M_PI/180);
    double phi2 = (num2->latitude) * (M_PI/180);

    double phiDelta = ((num2->latitude) - (num1->latitude)) * (M_PI/180);
    double lambdaDelta = ((num2->longitude) - (num1->longitude)) * (M_PI/180);

    double a = sin(phiDelta/2) * sin(phiDelta/2) +cos(phi1)* cos(phi2) * (sin(lambdaDelta/2) * sin(lambdaDelta/2));
    double c = 2 *atan2(sqrt(a), sqrt(1-a));
    double d = 6371e3*c;
 
	return d; // Return our calculated distance
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
    length = strlen(temp->name) + strlen(temp->value) + strlen("\t ");
    myString = malloc(sizeof(char) * (length+1));

    //strcpy info in the string 
    strcpy(myString,"\t");
    strcat(myString,temp->name);
    strcat(myString," ");
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

    myWptString = malloc(sizeof(char)*(len + 1)); 

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

    string = malloc(sizeof(char)*(size + 1));
    
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


/* Helper. Get all track segments */
List * getTrackSegmentList(const GPXdoc* doc) {
    NULLCHECK(doc)

    List *track_seg_list = initializeList(trackSegmentToString,deleteTrackSegment,compareTrackSegments);

    TrackSegment *track_seg = NULL;
    Track *temp_track = NULL;
    void *elem;

    ListIterator iter = createIterator(doc->tracks);

    while ((elem = nextElement(&iter)) != NULL) {

        temp_track = (Track *)elem;

        ListIterator iter = createIterator(temp_track->segments);

        for(int i = 0; i < getLength(temp_track->segments); i++) 
        {
            track_seg = (TrackSegment*)nextElement(&iter);
            insertBack(track_seg_list,track_seg);
        }
    }
    return track_seg_list;
}
int getNumSegments(const GPXdoc* doc) {
    if (doc == NULL) return -1;
    int num = 0;
    List *trackseg = getTrackSegmentList(doc);

    for(int i = 0; i < getLength(trackseg); i++) 
    {
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

/* Total number of GPXData elements in the document */
int getNumGPXData(const GPXdoc* doc) {
    if (doc ==NULL) return -1;
    int num =0;
    
    num = num + numData(getWaypoints(doc),"waypoint");
    num = num + numData(getAllTracks(doc),"track");
    num = num + numData(getAllRoutes(doc),"route");
    num = num + numData(getTrackSegmentList(doc), "trackSeg");

    return num;
}

int numData(List *list, char* type) {
    int num = 0;
    ListIterator iter = createIterator(list);
    
    /*Use the type to determine which length to get*/
    for(int i = 0; i < getLength(list); i++) {
        if(strcmp(type,"waypoint") == 0)
        {
            Waypoint* wpt = (Waypoint*)nextElement(&iter);
            num += getLength(wpt->otherData);
            if (strcmp(wpt->name,"") != 0 ) num++;
        } 
        else if(strcmp(type,"track") == 0) 
        {
            Track* trk = (Track*)nextElement(&iter);
            num += getLength(trk->otherData);
            if (strcmp(trk->name,"") != 0 ) num++;
        } 
        else if(strcmp(type,"route") == 0)
        {
            Route* rte = (Route*)nextElement(&iter);
            num += getLength(rte->otherData);
            num += numData(rte->waypoints, "waypoint");
            if (strcmp(rte->name,"") != 0 ) num++;

        }
        else if (strcmp(type,"trackSeg") == 0)
        {
            TrackSegment* trkseg = (TrackSegment*)nextElement(&iter);
            num += numData(trkseg->waypoints, "waypoint");

        }
    }
    return num;
}


float getTrackLen(const Track *tr) {
    return 0;
}


/** Function that returns the number routes with the specified length, using the provided tolerance 
 * to compare route lengths
 *@pre GPXdoc object exists, is not null
 *@post GPXdoc object exists, is not null, has not been modified
 *@return the number of routes with the specified length
 *@param doc - a pointer to a GPXdoc struct
 *@param len - search route length
 *@param delta - the tolerance used for comparing route lengths
**/
int numRoutesWithLength(const GPXdoc* doc, float len, float delta) {
    return 0;
}


/** Function that returns the number tracks with the specified length, using the provided tolerance 
 * to compare track lengths
 *@pre GPXdoc object exists, is not null
 *@post GPXdoc object exists, is not null, has not been modified
 *@return the number of tracks with the specified length
 *@param doc - a pointer to a GPXdoc struct
 *@param len - search track length
 *@param delta - the tolerance used for comparing track lengths
**/
int numTracksWithLength(const GPXdoc* doc, float len, float delta) {
    return 0;
}

/** Function that checks if the current route is a loop
 *@pre Route object exists, is not null
 *@post Route object exists, is not null, has not been modified
 *@return true if the route is a loop, false otherwise
 *@param route - a pointer to a Route struct
 *@param delta - the tolerance used for comparing distances between start and end points
**/
bool isLoopRoute(const Route* route, float delta) {
    return false;
}


/** Function that checks if the current track is a loop
 *@pre Track object exists, is not null
 *@post Track object exists, is not null, has not been modified
 *@return true if the track is a loop, false otherwise
 *@param track - a pointer to a Track struct
 *@param delta - the tolerance used for comparing distances between start and end points
**/
bool isLoopTrack(const Track *tr, float delta) {
    return false;
}


/** Function that returns all routes between the specified start and end locations
 *@pre GPXdoc object exists, is not null
 *@post GPXdoc object exists, is not null, has not been modified
 *@return a list of Route structs that connect the given sets of coordinates
 *@param doc - a pointer to a GPXdoc struct
 *@param sourceLat - latitude of the start location
 *@param sourceLong - longitude of the start location
 *@param destLat - latitude of the destination location
 *@param destLong - longitude of the destination location
 *@param delta - the tolerance used for comparing distances between waypoints 
*/
List* getRoutesBetween(const GPXdoc* doc, float sourceLat, float sourceLong, float destLat, float destLong, float delta) {
    return NULL;
}

/** Function that returns all Tracks between the specified start and end locations
 *@pre GPXdoc object exists, is not null
 *@post GPXdoc object exists, is not null, has not been modified
 *@return a list of Track structs that connect the given sets of coordinates
 *@param doc - a pointer to a GPXdoc struct
 *@param sourceLat - latitude of the start location
 *@param sourceLong - longitude of the start location
 *@param destLat - latitude of the destination location
 *@param destLong - longitude of the destination location
 *@param delta - the tolerance used for comparing distances between waypoints 
*/
List* getTracksBetween(const GPXdoc* doc, float sourceLat, float sourceLong, float destLat, float destLong, float delta) {
    return NULL;
}


//Module 3


/** Function to converting a Track into a JSON string
 *@pre Track is not NULL
 *@post Track has not been modified in any way
 *@return A string in JSON format
 *@param event - a pointer to a Track struct
 **/
char* trackToJSON(const Track *tr) {
    return " ";
}

/** Function to converting a Route into a JSON string
 *@pre Route is not NULL
 *@post Route has not been modified in any way
 *@return A string in JSON format
 *@param event - a pointer to a Route struct
 **/
char* routeToJSON(const Route *rt) {
    return " ";
}

/** Function to converting a list of Route structs into a JSON string
 *@pre Route list is not NULL
 *@post Route list has not been modified in any way
 *@return A string in JSON format
 *@param event - a pointer to a List struct
 **/
char* routeListToJSON(const List *list) {
    return " ";
}

/** Function to converting a list of Track structs into a JSON string
 *@pre Track list is not NULL
 *@post Track list has not been modified in any way
 *@return A string in JSON format
 *@param event - a pointer to a List struct
 **/
char* trackListToJSON(const List *list) {
    return " ";
}

/** Function to converting a GPXdoc into a JSON string
 *@pre GPXdoc is not NULL
 *@post GPXdoc has not been modified in any way
 *@return A string in JSON format
 *@param event - a pointer to a GPXdoc struct
 **/
char* GPXtoJSON(const GPXdoc* gpx) {
    return " ";
}



void addRoute(GPXdoc* doc, Route* rt) {

}

/** Function to converting a JSON string into an GPXdoc struct
 *@pre JSON string is not NULL
 *@post String has not been modified in any way
 *@return A newly allocated and initialized GPXdoc struct
 *@param str - a pointer to a string
 **/
GPXdoc* JSONtoGPX(const char* gpxString) {
    return NULL;
}

/** Function to converting a JSON string into an Waypoint struct
 *@pre JSON string is not NULL
 *@post String has not been modified in any way
 *@return A newly allocated and initialized Waypoint struct
 *@param str - a pointer to a string
 **/
Waypoint* JSONtoWaypoint(const char* gpxString) {
    return NULL;
}

/** Function to converting a JSON string into an Route struct
 *@pre JSON string is not NULL
 *@post String has not been modified in any way
 *@return A newly allocated and initialized Route struct
 *@param str - a pointer to a string
 **/
Route* JSONtoRoute(const char* gpxString) {
    return NULL;
}

void addWaypoint(Route *rt, Waypoint *pt) {

}

