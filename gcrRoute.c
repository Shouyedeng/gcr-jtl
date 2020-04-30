/* gcrRoute.c -
 *
 * The greedy router:  Top level procedures.
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
static char rcsid[] __attribute__ ((unused)) = "$Header: /usr/cvsroot/magic-8.0/gcr/gcrRoute.c,v 1.1.1.1 2008/02/03 20:43:50 tim Exp $";
#endif  /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/times.h>
#include <string.h>
#include <assert.h>

#include "router.h"
#include "magic.h"
#include "geometry.h"
#include "gcr.h"
#include "signals.h"
#include "malloc.h"
#include "styles.h"

int gcrRouterErrors;
extern int gcrStandalone;

/* Forward declarations */
void gcrRouteCol();
void gcrExtend();


/*
 * ----------------------------------------------------------------------------
 *
 * GCRroute --
 *
 * Top level for the greedy channel router.
 * Routes are already set up channel routing problem.
 *
 * Results:
 *	The return value is the number of errors found while routing
 *	this channel.
 *
 * Side effects:
 *	Modifies flag bits in the channel to show the presence of routing.
 *	Calls RtrChannelError when there are errors.
 *
 * ----------------------------------------------------------------------------
 */

int
GCRroute(ch, outfp)
    GCRChannel *ch;
	FILE *outfp;
{
    int i, density, netId;
    char mesg[256];
    GCRColEl *col;
    GCRPin *pin;
    GCRNet *net;
    
    //HJY
    //SigInterruptPending = FALSE;
    //printf("---start river routing\n"); 
    /* Try river-routing across the channel if possible */
    gcrRouterErrors = 0;
    //if (gcrRiverRoute(ch)){
    //	return (gcrRouterErrors);
    //}
    printf("---start build nets\n");
	fprintf(outfp, "---start build nets\n");
    gcrBuildNets(ch, outfp);
    printf("---finish build nets\n");
	fprintf(outfp, "---finish build nets\n");

    if (ch->gcr_nets == (GCRNet *) NULL){
        printf("ERROR: ch->gcr_nets==NULL\n");
		fprintf(outfp, "ERROR: ch->gcr_nets==NULL\n");
		return (gcrRouterErrors);

    }
	
	//find out how many net pins are on the right edge of the channel
    gcrSetEndDist(ch);
	density = gcrDensity(ch);
    printf("channel density (%d)  channel width (%d)\n", density, ch->gcr_width);
	fprintf(outfp, "channel density (%d)  channel width (%d)\n", density, ch->gcr_width);
/*  gcrPrDensity(ch, density);	/* Debugging */
    if (density > ch->gcr_width)
    {
		(void) sprintf(mesg, "Density (%d) > channel size (%d)",
	    	density, ch->gcr_width);
		//RtrChannelError(ch, ch->gcr_width, ch->gcr_length, mesg, NULL);
		printf("Error: density > channelwidth\n");
		fprintf(outfp, "Error: density > channelwidth\n");
    }

    gcrInitCollapse(ch->gcr_width + 2);
    gcrSetFlags(ch);

    /* Process the first column */
    gcrInitCol(ch, ch->gcr_lPins);
    gcrExtend(ch, 0, outfp);
    gcrPrintCol(ch, 0, GcrShowResult); 

    /* Process subsequent columns */
    for (i = 1; i <= ch->gcr_length; i++)
    {
	//if (SigInterruptPending)
	//    goto bottom;
		gcrRouteCol(ch, i, outfp);
    }
    //hjy
    assert(GcrShowEnd);
    //printf("start dump result\n");
    //gcrDumpResult(ch, GcrShowEnd);
    //printf("finish dump result\n");


    /* Process errors at the end */
    col = ch->gcr_lCol;
    pin = ch->gcr_rPins;
    for (i = 1; i <= ch->gcr_width; i++, col++, pin++){
		if(pin->gcr_pId != (GCRNet *) NULL)
		if (col->gcr_h != pin->gcr_pId && pin->gcr_pId->routeflag == 1)
		{
	    	netId = col->gcr_h ? col->gcr_h->gcr_Id : pin->gcr_pId->gcr_Id;
	    	//RtrChannelError(ch, ch->gcr_length, i,
			//		"Can't make end connection", netId);
	    	printf("Error: Cannot make end connection for net %d\n", netId);
			fprintf(outfp,"Error: Cannot make end connection for net %d\n", netId);
	    	gcrRouterErrors++;
		}
	}
	ch->failed_net = gcrRouterErrors;

bottom:
    /* For debugging: print channel on screen */
    //gcrDumpResult(ch, GcrShowEnd);

    /*
     * We have to free up the nets here, since callers may re-arrange
     * the channel and cause the net structure to become invalid
     * anyway.
     */
	//DumpNetPath(ch, outfp);
#if 0    
	for (net = ch->gcr_nets; net; net = net->gcr_next){
		for(int i=0; i < ch->gcr_length +2; i++)
		{
			freeMagic((void *)net->path[i]);
		}
		freeMagic((void *)net->path);
		freeMagic((char *) net);
	}
		
    
	ch->gcr_nets = NULL;
#endif
    return (gcrRouterErrors);
}

/*
 * ----------------------------------------------------------------------------
 *
 * gcrRouteCol --
 *
 * Route the given column in the channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets flags in the channel structure to show where routing
 *	is to be placed.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrRouteCol(ch, indx,outfp)
    GCRChannel *ch;
    int indx;		/* Index of column being routed. */
	FILE *outfp;
{
    GCRNet **gcrClassify(), **list;
    GCRColEl *col;
    int count;

    /* Make feasible top and bottom connections */
    gcrCheckCol(ch, indx, "Start of gcrRouteCol");
    gcrFeasible(ch, indx);
    gcrCheckCol(ch, indx, "After feasible connections");

    /* Here I should vacate terminating tracks */
    if (GCRNearEnd(ch, indx) &&
	    (GCREndDist < ch->gcr_length || !GCRNearEnd(ch, indx - 1)))
	gcrMarkWanted(ch);

    /* Collapse split nets in the pattern that frees the most tracks */
    gcrCollapse(&ch->gcr_lCol, ch->gcr_width, 1, ch->gcr_width, 0);
    gcrPickBest(ch);
    gcrCheckCol(ch, indx, "After collapse");

    col = ch->gcr_lCol;

    /* Reduce the range of split nets */
    gcrReduceRange(col, ch->gcr_width);
    gcrCheckCol(ch, indx, "After reducing range of split nets");

    /* Vacate obstructed tracks.  Split to make multiple end connections */
    gcrVacate(ch, indx);

    /* Raise rising and lower falling nets */
    list = gcrClassify(ch, &count);
    gcrCheckCol(ch, indx, "After classifying nets");
    gcrMakeRuns(ch, indx, list, count, TRUE);
    gcrCheckCol(ch, indx, "After making rising/falling runs");

    gcrCheckCol(ch, indx, "After vacating");
    if (GCRNearEnd(ch, indx))
    {
		gcrUncollapse(ch, &ch->gcr_lCol, ch->gcr_width, 1, ch->gcr_width, 0);
		gcrPickBest(ch);
    }
    gcrCheckCol(ch, indx, "After uncollapse");

    /* Extend active tracks to the next column.  Place contacts */
    gcrExtend(ch, indx, outfp);
    gcrCheckCol(ch, indx, "After widen and extend");
    gcrPrintCol(ch, indx, GcrShowResult); 
}

/*
 * ----------------------------------------------------------------------------
 *
 * gcrExtend --
 *
 * Extend dangling wires to the next column.
 * Don't extend off the end of the channel if the wrong connection
 * would be made.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets bits in the result array for the channel.  Where there
 *	are blockages in the next column, adds contacts to blocked
 *	tracks for a layer switch.  Clears the vertical wiring for
 *	the new column.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrExtend(ch, currentCol, outfp)
    GCRChannel *ch;	/* Channel being routed */
    int currentCol;	/* Column that has just been completed */
	FILE *outfp;
{
    short *res = ch->gcr_result[currentCol];
    GCRColEl *col = ch->gcr_lCol;
    short *prev = (short *) NULL, *next = (short *) NULL, prevRow = 0;
    bool hasNext, hasPrev;
    int i;
	GCRNet *v_net;
	GCRNet *h_net;
	GCRNet *varray[100]; //assume width < 100
	GCRNet *harray[100];

	for(i = 0; i < 100; ++i) {
		varray[i] = NULL;
		harray[i] = NULL;
	}

    ASSERT(ch->gcr_result, "gcrExtend: ");
    if (currentCol > 0) 
		prev = ch->gcr_result[currentCol - 1];
    if (currentCol <= ch->gcr_length) 
		next = ch->gcr_result[currentCol + 1];

    /*
     * Consider each track, including the pseudo-track at the
     * bottom (0) of the channel, but not the one at the top
     * (ch->gcr_width).
     */
	printf("\n\n--extend col: %d---\n", currentCol);
	fprintf(outfp, "\n\n--extend col: %d---\n", currentCol);

	for (i = 1; i <= ch->gcr_width; i++){
		if(col[i].gcr_v != NULL){
			printf(" current col %d row %d, gcr_v net id %d, path %d\n", currentCol, i, col[i].gcr_v->gcr_Id, col[i].gcr_v->path[currentCol][i]);
			fprintf(outfp, " current col %d row %d, gcr_v net id %d, path %d\n", currentCol, i, col[i].gcr_v->gcr_Id, col[i].gcr_v->path[currentCol][i]);
			varray[i] = col[i].gcr_v;
		}
		if(col[i].gcr_h != NULL){
			printf(" current col %d row %d, gcr_h net id %d, path %d\n", currentCol, i, col[i].gcr_h->gcr_Id, col[i].gcr_h->path[currentCol][i]);
			fprintf(outfp, " current col %d row %d, gcr_h net id %d, path %d\n", currentCol, i, col[i].gcr_h->gcr_Id, col[i].gcr_h->path[currentCol][i]);
			harray[i] = col[i].gcr_h;
		}

		if(prev){
			printf("prev[%d] %d\n", i, prev[i]);
			fprintf(outfp, "prev[%d] %d\n", i, prev[i]);
		}


	}


    for (i = 0; i <= ch->gcr_width; i++)
    {
		
		if (col[1].gcr_v == col->gcr_v && col->gcr_v)
		{
	    	/* Track extends upwards */
	    	res[0] |= GCRU;
	    	if (i == ch->gcr_width) res[1] |= GCRU;
	    	if (col->gcr_flags & GCRCC) res[0] |= GCRX;
	    	if (col[1].gcr_flags & GCRCC) res[1] |= GCRX;
		}

		/* Don't process track if not occupied by a real net */
		hasPrev = prev && (*prev & GCRR);
		if (col->gcr_h == (GCRNet *) NULL)
		{
	    	if (currentCol == 0) res[0] &= ~GCRR;
	    	if (hasPrev) res[0] |= GCRX;
	    	col->gcr_v = 0;
		}
		else
		{
	    /* Extend net if split or another pin exists in this channel */
		    hasNext =  col->gcr_hi != EMPTY
			    || col->gcr_lo != EMPTY
			    || GCRPin1st(col->gcr_h);

	    	if (col->gcr_v == col->gcr_h && (hasPrev || hasNext))
				res[0] |= GCRX;

	    	/* Clear vertical wiring */
	    	col->gcr_v = 0;

	   		/* Terminate unsplit nets with no pins after the current column */
	    	if (!hasNext) col->gcr_h = (GCRNet *) NULL;
	   		else if (col->gcr_flags & GCRTE)
	    	{
			/*
			 * If the track should be extended but can't due to a
			 * hard obstacle, then print a message and terminate it.
		 	*/
			//RtrChannelError(ch, currentCol, i,
			//    "Can't extend track through obstacle", col->gcr_h->gcr_Id);
	        	printf("Error: cannot extend track through obstacle\n");
				fprintf(outfp, "Error: cannot extend track through obstacle\n");
				gcrRouterErrors++;
				col->gcr_h = (GCRNet *) NULL;
	    	}
	    	else if (currentCol == ch->gcr_length && i
				&& ch->gcr_rPins[i].gcr_pId == (GCRNet *) NULL)
	    	{
			/* If track about to make a bad connection, don't extend */
			//RtrChannelError(ch, currentCol, i,
			//    "Can't extend track to bad connection", col->gcr_h->gcr_Id);
				printf("Error: cannot extend track to bad connection\n");
				fprintf(outfp, "Error: cannot extend track to bad connection\n");
				col->gcr_h = (GCRNet *) NULL;
				gcrRouterErrors++;
	    	}
	   		else
	    	{
			/* Extend the net into the next column */
				res[0] |= GCRR;
				if (currentCol == ch->gcr_length) *next |= GCRR;
	    	}	

	    	/* Contact in next col if GCRTC */
	    	if (*next & GCRTC) col->gcr_v = col->gcr_h;
		}
		//printf("track %d, res[0] is %d, res[1] is %d, prevRow %d\n", i, res[0], res[1], prevRow);
		
		if(currentCol > 0 && i > 0){
		    v_net = varray[i];
			h_net = harray[i];
			printf("row %d\n", i);
			fprintf(outfp, "row %d\n", i);
			if(v_net && h_net){
            	printf(" vnet id %d hnet id %d  ", v_net->gcr_Id, h_net->gcr_Id);
				fprintf(outfp, " vnet id %d hnet id %d  ", v_net->gcr_Id, h_net->gcr_Id);			
				if(v_net->gcr_Id == h_net->gcr_Id) {
				    
					if((v_net->direction & NU) && (v_net->direction & NR) ){
						if((prevRow) & GCRU) // row i-1 is vertical
							v_net->path[currentCol][i] = JUR;
						else
							v_net->path[currentCol][i] = JRU;
					}
					else if((v_net->direction & NU) && (v_net->direction & NL) )
		    			if ((prevRow) & GCRU)
							v_net->path[currentCol][i] = JUL;
						else
							v_net->path[currentCol][i] = JLU;
					else if((v_net->direction & ND) && (v_net->direction & NR) )
		    			if (ch->gcr_result[currentCol-1] && (ch->gcr_result[currentCol-1][i] & GCRR)){
							printf("prev[%d] %d, ch prev %d\n", i, prev[i], ch->gcr_result[currentCol-1][i]);
							fprintf(outfp, "prev[%d] %d, ch prev %d\n", i, prev[i], ch->gcr_result[currentCol-1][i]);
							v_net->path[currentCol][i] = JRD;
					    
						}else
							v_net->path[currentCol][i] = JDR;
					else if((v_net->direction & ND) && (v_net->direction & NL) )
		    			if (ch->gcr_result[currentCol-1] && (ch->gcr_result[currentCol-1][i] & GCRR))
							v_net->path[currentCol][i] = JDL;
					    else
							v_net->path[currentCol][i] = JLD;
					else if(v_net->direction & ND )
						v_net->path[currentCol][i] = JD;
					else if(v_net->direction & NU )
						v_net->path[currentCol][i] = JU;
					else if(v_net->direction & NL )
						v_net->path[currentCol][i] = JL;
					else if(v_net->direction & NR )
						v_net->path[currentCol][i] = JR;
					else{
						printf("Error: in gcrRoute.c when set net path\n");
						exit(-1);	
					}

					printf("direc %d\n", v_net->path[currentCol][i]);
					fprintf(outfp, "direc %d\n", v_net->path[currentCol][i]);	
				}else{ //two nets crosses
				
					if((h_net->direction & NR) && (v_net->direction & NU)){
						h_net->path[currentCol][i] = JXUR;
						v_net->path[currentCol][i] = JXUR;	

						printf("--NR  NU----\n");
						fprintf(outfp, "--NR  NU----\n");
					} else if((h_net->direction & NR) && (v_net->direction & ND)){
						h_net->path[currentCol][i] = JXDR;
						v_net->path[currentCol][i] = JXDR;	
						printf("--NR  ND----\n");
						fprintf(outfp, "--NR  ND----\n");
					}else if((h_net->direction & NL) && (v_net->direction & NU)){
						h_net->path[currentCol][i] = JXUL;
						v_net->path[currentCol][i] = JXUL;	
						printf("--NL  NU----\n");
						fprintf(outfp, "--NL  NU----\n");
					}else if((h_net->direction & NL) && (v_net->direction & ND)){
						h_net->path[currentCol][i] = JXDL;
						v_net->path[currentCol][i] = JXDL;	
						printf("--NL  ND----\n");
						fprintf(outfp, "--NL  ND----\n");
					}else{
						printf("Error: weird\n");
						fprintf(outfp, "Error: weird\n");
					}
					printf("direc %d\n", v_net->path[currentCol][i]);
					fprintf(outfp, "direc %d\n", v_net->path[currentCol][i]);	
				}
			
			} else if(v_net && !h_net){
            	printf(" vnet id %d hnet==null  ", v_net->gcr_Id); 
				fprintf(outfp, " vnet id %d hnet==null  ", v_net->gcr_Id);
				//to solve x and GCRR
				if(ch->gcr_result[currentCol-1] && (ch->gcr_result[currentCol-1][i] & GCRR)){
					
					if((v_net->direction & NU) && (v_net->direction & NR))
						v_net->path[currentCol][i] = JRU;	
					else if((v_net->direction & NU) && (v_net->direction & NL))
						v_net->path[currentCol][i] = JUL;	
					else if((v_net->direction & ND) && (v_net->direction & NR))
						v_net->path[currentCol][i] = JRD;	
					else if((v_net->direction & ND) && (v_net->direction & NL))
						v_net->path[currentCol][i] = JLD;	
				
				}else{
							
					if(v_net->direction & NU)
						v_net->path[currentCol][i] = JU;	
					else
						v_net->path[currentCol][i] = JD;	
				}
				printf("direc %d\n", v_net->path[currentCol][i]);	
                fprintf(outfp, "direc %d\n", v_net->path[currentCol][i]);
			} else if(!v_net && h_net){
            	printf(" vnet==null hnet id %d  ", h_net->gcr_Id);	
				fprintf(outfp, " vnet==null hnet id %d  ", h_net->gcr_Id);		
				if(h_net->direction & NL)
					h_net->path[currentCol][i] = JL;	
				else
					h_net->path[currentCol][i] = JR;	
				printf("direc %d\n", h_net->path[currentCol][i]);	
				fprintf(outfp, "direc %d\n", h_net->path[currentCol][i]);
			}else{
				printf("vnet==null && hnet==null\n");
				fprintf(outfp, "vnet==null && hnet==null\n");
				//ASSERT(!v_net && !h_net, "Error: v_net and h_net are not null");
			}

		} //end if
        
		prevRow = res[0];
		
		if (prev) prev++;
		if (next) col->gcr_flags = *next++;
		else col->gcr_flags= 0;
				
		res++;
		col++;
    } //end for
	
    col->gcr_v = 0;
    col->gcr_flags = 0;
}
