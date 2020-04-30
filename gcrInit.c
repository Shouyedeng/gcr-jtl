/* gcrInit.c -
 *
 *	Initialization for the greedy router.
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1985, 1990 Regents of the University of California. * 
 *     * Permission to use, copy, modify, and distribute this              * 
 *     * software and its documentation for any purpose and without        * 
 *     * fee is hereby granted, provided that the above copyright          * 
 *     * notice appear in all copies.  The University of California        * 
 *     * makes no representations about the suitability of this            * 
 *     * software for any purpose.  It is provided "as is" without         * 
 *     * express or implied warranty.  Export of this software outside     * 
 *     * of the United States of America may require an export license.    * 
 *     *********************************************************************
 */

#ifndef lint
static char rcsid[] __attribute__ ((unused)) = "$Header: /usr/cvsroot/magic-8.0/gcr/gcrInit.c,v 1.1.1.1 2008/02/03 20:43:50 tim Exp $";
#endif  /* not lint */

#include <stdio.h>

#include "magic.h"
#include "geometry.h"
#include "hash.h"
#include "heap.h"
#include "tile.h"
#include "database.h"
#include "router.h"
#include "gcr.h"
#include "malloc.h"

int   GCRSteadyNet	=1;	/*Values get imported from main*/
int   GCREndDist  	=1;
int   GCRMinJog   	=1;
int   GCRsplitAt  	=0;
bool  GcrShowResult	=FALSE;
bool  GcrNoCheck   	=TRUE;
bool  GcrDebug		=FALSE;
#ifndef	lint
float GCRObstDist 	= 0.7;
#else
float GCRObstDist;	/* Sun lint brain death */
#endif	/* lint */
float RtrEndConst = 1.0;

/* Forward declarations */
void gcrLinkPin();


/*
 * ----------------------------------------------------------------------------
 *
 * gcrSetEndDist --
 *
 * Set the global variable specifying how soon the router starts to get
 * nets into tracks for end connections.
 *
 * Get the number of end connections as a percentage of the width of
 * the channel.  Add extra for multipin end connections.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes GCREndDist.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrSetEndDist(ch)
    GCRChannel *ch;	/* The channel to be routed */
{
    int rightTotal, multiTotal;
    GCRNet *net;
    GCRPin *pin;
    int count;

    /*
     * Do this the easy way.
     * Enumerate each net and look at its pins, starting
     * from the rightmost.
     */
    rightTotal = 0;
    multiTotal = 0;
    for (net = ch->gcr_nets; net; net = net->gcr_next)
    {
		count = 0;
		for (pin = net->gcr_rPin; pin; pin = pin->gcr_pPrev, count++){
	    	if (pin->gcr_x <= ch->gcr_length){
				break;
			}
		}
		rightTotal += count;
		if (count > 1){
	    	multiTotal++;
		}
    }

    GCREndDist = (multiTotal/2 + rightTotal/4) * RtrEndConst;
    if (GCREndDist < 1) GCREndDist = 1;
}

#if 0
//set net source and sink coordinate
void set_net_terminal_coord(pin, ht, x, y)
	GCRPin *pin;
    HashTable *ht;
	int x;
	int y;
{
    GCRNet *net;
    HashEntry *hEntry;

    if(pin->gcr_pSeg == 0) return;
    printf("try to find net %d\n", pin->gcr_pSeg);
	ASSERT(ht, "hash table is not empty");
	hEntry = HashFind(ht, (char *) &(pin->gcr_pSeg));
    net = (GCRNet *) HashGetValue(hEntry);
    if (net == (GCRNet *) NULL){
		printf("net %d not found in hash table\n", pin->gcr_pSeg);
	
	}
	if(pin->isSrc){
		net->driver.p_x = x;
		net->driver.p_y = y;
	}else{
		net->receiver.p_x = x;
		net->receiver.p_y = y;
	}

}
#endif




/*
 * ----------------------------------------------------------------------------
 *
 * gcrBuildNets --
 *
 * Scan the top, bot, left, and right pin arrays, setting pointers to
 * the left and right pins for each net.  Sets pointers within the
 * pin arrays to point to next and previous pins.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates and initializes storage for nets.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrBuildNets(ch, outfp)
    GCRChannel	* ch;
	FILE *outfp;
{
    HashTable ht;
    int i;
    
	GCRPin *pin;
    GCRNet *net;
    HashEntry *hEntry;
    
	bool x_equal, y_equal, to_right, to_up, to_down;

    /*
     * Initialize the hash table that maps net ids to GCRNet structs.
     * The table is keyed by 2 words: seg/net.
     */
    HashInit(&ht, 256, 2);

    for (i = 1; i <= ch->gcr_width; i++){
		gcrLinkPin(&ch->gcr_lPins[i], &ht, ch);
    	
	}

    for (i = 1; i <= ch->gcr_length; i++)
    {
		gcrLinkPin(&ch->gcr_bPins[i], &ht, ch);
		gcrLinkPin(&ch->gcr_tPins[i], &ht, ch);
    }

    for (i = 1; i <= ch->gcr_width; i++){
		gcrLinkPin(&ch->gcr_rPins[i], &ht, ch);
	}
    
	for (net = ch->gcr_nets; net; net = net->gcr_next){
		
		x_equal = net->driver.p_x == net->receiver.p_x; 
		y_equal = net->driver.p_y == net->receiver.p_y; 
		to_right = net->driver.p_x < net->receiver.p_x; 
		to_up = net->driver.p_y < net->receiver.p_y; 
		to_down = net->driver.p_y > net->receiver.p_y; 
	    
		if(x_equal){
			if(to_up) net->direction |= NU;
		    else if(to_down)  net->direction |= ND;
			else 
			{
			printf("Error: net src location (%d, %d) equals sink location (%d, %d)\n", net->driver.p_x, net->driver.p_y, net->receiver.p_x, net->receiver.p_y);
			fprintf(outfp,"Error: net src location (%d, %d) equals sink location (%d, %d)\n", net->driver.p_x, net->driver.p_y, net->receiver.p_x, net->receiver.p_y);
			}
		}else if(to_right){
			net->direction |= NR;
			if(to_up) net->direction |= NU;
			else if(to_down) net->direction |= ND;
		
		}else{
			net->direction |= NL;
			if(to_up) net->direction |= NU;
			else if(to_down) net->direction |= ND;
		}
		if(net->hasreceiver == 1 && net->hasdriver == 1)
		{
			net->routeflag = 1;
			printf("net %d direction %d\n", net->gcr_Id, net->direction);
			printf("net src location (%d, %d) sink location (%d, %d)\n", net->driver.p_x, net->driver.p_y, net->receiver.p_x, net->receiver.p_y);
			fprintf(outfp, "net %d direction %d\n", net->gcr_Id, net->direction);
			fprintf(outfp, "net src location (%d, %d) sink location (%d, %d)\n", net->driver.p_x, net->driver.p_y, net->receiver.p_x, net->receiver.p_y);
		}
		else
		{
			net->routeflag = 0;
			printf("the driver or receiver of the net (%d) is missing .No need to route.\n", net->gcr_Id);
			fprintf(outfp, "the driver or receiver of the net(%d) is missing .No need to route.\n", net->gcr_Id);
		}
		
	}
   

    HashKill(&ht);
}

/*
 * ----------------------------------------------------------------------------
 *
 * gcrLinkPin --
 *
 * Establish the forward and backward links for a pin on the edge of
 * a channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The pin is added to the end of the list for its net.  A
 *	new net structure is created if one doesn't already
 *	exist.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrLinkPin(pin, ht, ch)
    GCRPin *pin;
    HashTable *ht;
    GCRChannel *ch;
{
    GCRNet *net;
    GCRNet *net2;
    GCRNet *gcrNewNet();
    HashEntry *hEntry;

    int i,j,k;
    /*
     * GCR_BLOCKEDNETID means that the crossing was blocked.
     * Restore these crossings to "empty".
     */
    //if (pin->gcr_pId == GCR_BLOCKEDNETID) pin->gcr_pId = (GCRNet *) NULL;

    /* Skip empty pins */
    if(pin->gcr_pSeg == 0) return;
    
	/* Find the 2-word seg/net key */
    hEntry = HashFind(ht, (char *) &(pin->gcr_pSeg));
    net = (GCRNet *) HashGetValue(hEntry);
    if (net == (GCRNet *) NULL)
    {
		/* New net */
		net = (GCRNet *) mallocMagic((unsigned) (sizeof (GCRNet)));
		HashSetValue(hEntry, (char *) net);
		//net->gcr_Id = (spointertype) pin->gcr_pSeg;
		net->gcr_Id =  pin->gcr_pSeg;
		net->hasreceiver = 0;
		net->hasdriver = 0;
		net->routeflag = 0;
		net->driver.p_x = -1; net->driver.p_y = -1;
		net->receiver.p_x = -1; net->receiver.p_y = -1;
	    //allocate memory for jtl path
		net->path = (unsigned short **) mallocMagic((unsigned) ((ch->gcr_length + 2) * sizeof (short *)));
    	for (i = 0; i < ch->gcr_length + 2; i++)
    	{
			/* allocate the column */
			net->path[i] = (unsigned short *) mallocMagic((unsigned) ((ch->gcr_width + 2) * sizeof (short)));
		}
		for(i = 0; i < ch->gcr_length + 2; i++){
			for(j = 0; j < ch->gcr_width + 2; j++){
				net->path[i][j] = 0;
			}
		}
		net->direction &= 0;
		if(pin->isSrc){
			net->hasdriver = 1;
			net->driver.p_x = pin->gcr_x;
			net->driver.p_y = pin->gcr_y;
		}else{
			net->hasreceiver = 1;
			net->receiver.p_x = pin->gcr_x;
			net->receiver.p_y = pin->gcr_y;
		}
		
		
		//net->gcr_Id = (spointertype) pin->gcr_pId;
    	/* Link net onto channel net list */
		net->gcr_next = ch->gcr_nets;
		ch->gcr_nets = net;

		/* Link pin onto net pin list */
		net->gcr_lPin = net->gcr_rPin= pin;
		pin->gcr_pPrev = (GCRPin *) NULL;
        printf("new net seg %d\n", pin->gcr_pSeg);
		
		
    }
    else
    {
		/* Old net.  Add to end of net's list. */ 
		if(pin->isSrc){
			net->hasdriver = 1;
			net->driver.p_x = pin->gcr_x;
			net->driver.p_y = pin->gcr_y;
		}else{
			net->hasreceiver = 1;
			net->receiver.p_x = pin->gcr_x;
			net->receiver.p_y = pin->gcr_y;
		}
		net->gcr_rPin->gcr_pNext = pin;
		pin->gcr_pPrev = net->gcr_rPin;
		net->gcr_rPin = pin;
        printf("old net seg %d\n", pin->gcr_pSeg);
    }
    pin->gcr_pId = net;
    pin->gcr_pNext = (GCRPin *) NULL;
   
    printf("build_nets pSeg %d netid %d\n", pin->gcr_pSeg, net->gcr_Id);
	#if 0	
		hEntry = HashFind(ht, (char *) &(pin->gcr_pSeg));
    	net2 = (GCRNet *) HashGetValue(hEntry);
    	if (net2 == (GCRNet *) NULL || net != net2)
    	{

			printf("--world  net %d can not found\n", pin->gcr_pSeg);
		}
    #endif
}

/*
 * ----------------------------------------------------------------------------
 *
 * gcrUnlinkPin --
 *
 * Remove a pin from the pin list for a net.  The pin had better
 * be the first pin of its net.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The pin is removed from the list associated with its net.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrUnlinkPin(pin)
    GCRPin *pin;
{
    GCRNet *net;

    if (net = pin->gcr_pId)
    {
	ASSERT(pin == net->gcr_lPin, "gcrUnlinkPin");
	net->gcr_lPin = pin->gcr_pNext;
	if (pin->gcr_pNext)
	    pin->gcr_pNext->gcr_pPrev = pin->gcr_pPrev;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * gcrDensity --
 *
 * Determine the channel density at every column.  Starting at the left
 * end of the channel with density=0, the density of the next column is
 * the current density plus the number of nets with first pin in the
 * current column minus the number of nets with last pin in the previous
 * column.
 *
 * Must be called AFTER gcrBuildNets.
 *
 * Results:
 *	Returns the maximum column density.
 *
 * Side effects:
 *	Allocates storage for the density array GCRDensity and stores densities.
 *
 * ----------------------------------------------------------------------------
 */

int
gcrDensity(ch)
    GCRChannel *ch;
{
    int density, i, last, maxVal;
    unsigned lenWds;

    last = 0;
    density = 0;

    for (i = 1; i <= ch->gcr_width; i++)
	if (ch->gcr_lPins[i].gcr_pId)
	if(ch->gcr_lPins[i].gcr_pId->routeflag == 1)
	{
	    if (Is1stPin(&ch->gcr_lPins[i])) density++;
	    if (IsLstPin(&ch->gcr_lPins[i])) last++;
	}

    lenWds = ch->gcr_length + 2;
    if (ch->gcr_density == (int *) NULL)
    {
	ch->gcr_density = (int *) mallocMagic((unsigned) (lenWds * sizeof (int)));
    }
    ch->gcr_density[0] = maxVal = density;

    for (i = 1; i <= ch->gcr_length; i++)
    {
	density = density - last;
	last = 0;

	if (ch->gcr_tPins[i].gcr_pId)
	if(ch->gcr_tPins[i].gcr_pId->routeflag == 1)
	{
	    if (Is1stPin(&ch->gcr_tPins[i])) density++;
	    else if (IsLstPin(&ch->gcr_tPins[i])) last++;
	}

	if (ch->gcr_bPins[i].gcr_pId)
	if(ch->gcr_bPins[i].gcr_pId->routeflag == 1)
	{
	    if (Is1stPin(&ch->gcr_bPins[i])) density++;
	    else if (IsLstPin(&ch->gcr_bPins[i]))
	    {
		if (ch->gcr_tPins[i].gcr_pId == ch->gcr_bPins[i].gcr_pId)
		    density--;
		else last++;
	    }
	}

	ch->gcr_density[i] = density;
	if (density > maxVal)
	    maxVal = density;
    }

    return (maxVal);
}

/*
 * ----------------------------------------------------------------------------
 *
 * gcrInitCol --
 *
 * Initialize the column structure to hold the configuration at
 * the left side of the channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Pointers are updated in the column data structure.
 *	Changes the track field of each net struct.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrInitCol(ch, edgeArray)
    GCRChannel *ch;
    GCRPin *edgeArray;	/* Nets at left edge of channel if non-NULL */
{
    GCRNet *net;
    GCRColEl *col;
    int i, widWds;

    col = ch->gcr_lCol;

    /* Assign column tracks using an optionally provided pin array */
    if (edgeArray)
    {
		col[0].gcr_h = (GCRNet *) NULL;
		for (i = 1; i <= ch->gcr_width; i++)
		{
			
	    	col[i].gcr_h = edgeArray[i].gcr_pId;
			if(col[i].gcr_h != (GCRNet *) NULL)
				if(col[i].gcr_h->routeflag == 0)
					col[i].gcr_h = (GCRNet *) NULL;
	    	/* Delete pin from net's pin list */
	    	gcrUnlinkPin(&edgeArray[i]);
		}
		col[ch->gcr_width+1].gcr_h = (GCRNet *) NULL;
    }

    /* Initialize the hi and lo pointers for the column data structure */
    for (net = ch->gcr_nets; net; net = net->gcr_next)
		net->gcr_track = EMPTY;

    widWds = ch->gcr_width + 1;
    for (i = 0; i <= widWds; i++)
    {
		col[i].gcr_v = (GCRNet *) NULL;
		col[i].gcr_hi = EMPTY;
		col[i].gcr_lo = EMPTY;
		col[i].gcr_hOk =FALSE;
		col[i].gcr_lOk =FALSE;
		col[i].gcr_wanted = (GCRNet *) NULL;
		col[i].gcr_flags = 0;
		net = col[i].gcr_h;
		if (net) //no running
		{
	    	if (net->gcr_track == EMPTY){ /* 1st track */
				col[i].gcr_h->gcr_track = i;
			}else {
				col[i].gcr_lo = net->gcr_track;
				col[net->gcr_track].gcr_hi = i;
				net->gcr_track = i;
	    	}
		}
    }

    /* Mark tracks needed to make some end connection */
    for (i = 1; i <= ch->gcr_width; i++)
		gcrWanted(ch, i, 0);
}
