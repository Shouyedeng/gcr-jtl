#include <stdio.h>

#include "magic.h"
#include "geometry.h"
#include "gcr.h"
#include "router.h"
#include "tile.h"

int RtrGridSpacing = 1;


int main(){

    char* inFile = "gcr_sbox_conn.echo";
    //char* inFile = "gcr_sbox_conn_error.echo";
    GCRRouteFromFile(inFile);

    return 0;
}

