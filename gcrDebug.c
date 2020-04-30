/* gcrDebug.c -
 *
 * The greedy router, debug routines.
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
static char rcsid[] __attribute__ ((unused)) = "$Header: /usr/cvsroot/magic-8.0/gcr/gcrDebug.c,v 1.1.1.1 2008/02/03 20:43:50 tim Exp $";
#endif  /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/times.h>
#include <string.h>

#include "magic.h"
#include "geometry.h"
#include "textio.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "gcr.h"
#include "heap.h"
#include "router.h"
#include "malloc.h"

int gcrViaCount;
bool GcrShowEnd = TRUE;//FALSE;
bool GcrShowMap = FALSE;
int gcrStandalone = FALSE; /*Flag to control standalone/integrated operation*/

/* Forward declarations */
void gcrDumpResult();
void gcrStats();
void gcrShowMap();
bool gcrMakeChannel();




/*
 * ----------------------------------------------------------------------------
 *
 * GCRRouteFromFile --
 *
 * Reads a routing problem from the named file and performs the routing.
 *
 * Results:
 *	Returns a pointer to the routed channel.
 *
 * Side effects:
 *	Allocates memory.
 *
 * ----------------------------------------------------------------------------
 */
#if 0
GCRChannel **
GCRRouteFromFile(fname, outname)
    char *fname;
	char *outname;
{
    static Point initOrigin = { 0, 0 };
    struct tms tbuf1, tbuf2;
    GCRChannel **ch;
    Transform trans;
    FILE *fp;FILE *outfp;
    Rect box;
    int j,chan_num;
	char c;
    fp = fopen(fname, "r");
    if (fp == NULL)
    {
		perror(fname);
		return ((GCRChannel **) NULL);
    }

	outfp = fopen(outname,"w");
	if (outfp == NULL)
	{
		perror(outfp);
		return((GCRChannel **) NULL);
	}

	c = getc(fp);
    if (c != '*')
    {
		printf("Old-style channel format no longer supported.\n");
		fprintf(outfp, "Old-style channel format no longer supported.\n");
		return (FALSE);
    }
	if (fscanf(fp, "%d", &chan_num) != 1)
    {
		printf("Format error in input file channel number.\n");
		fprintf(outfp,"Format error in input file channel number.\n");
		return (FALSE);
    }

	printf("The number of channel is %d\n",chan_num);
	fprintf(outfp, "The number of channel is %d\n",chan_num);
	
	
	for(int i=0; i<chan_num; i++)
	{
		ch[i] = (GCRChannel *) mallocMagic((unsigned) (sizeof (GCRChannel)));
    	ch[i]->gcr_type = CHAN_NORMAL;
	/*     ch->gcr_area = box; */
    	ch[i]->gcr_transform = GeoIdentityTransform;
    	ch[i]->gcr_lCol = (GCRColEl *) NULL;
    	ch[i]->gcr_nets = (GCRNet *) NULL;
    	ch[i]->gcr_result = (short **) NULL;
    	ch[i]->gcr_origin = initOrigin;

		printf("channel[%d] \n",i+1);
		fprintf(outfp,"channel[%d] \n",i+1);
    	if (!gcrMakeChannel(ch[i], fp, outfp))
    	{
			printf("Couldn't initialize channel routing problem\n");
			fprintf(outfp,"Couldn't initialize channel routing problem\n");
			(void) fclose(fp);
			(void) fclose(outfp);
			freeMagic((char *) ch[i]);
			return ((GCRChannel **) NULL);
    	}
	}

    (void) fclose(fp);
	
	for (int i = 0; i < chan_num; i++)
	{
		printf("\n\nchannel[%d] \n",i+1);
		fprintf(outfp,"\n\nchannel[%d] \n",i +1);

		ch[i]->gcr_lCol = (GCRColEl *)mallocMagic((unsigned)((ch[i]->gcr_width + 2) * sizeof(GCRColEl)));

		times(&tbuf1);

		(void)GCRroute(ch[i], outfp);
 
		times(&tbuf2);

		printf("Time   :  %5.2fu  %5.2fs\n", (tbuf2.tms_utime - tbuf1.tms_utime) / 60.0, (tbuf2.tms_stime - tbuf1.tms_stime) * 60);
		fprintf(outfp,"Time   :  %5.2fu  %5.2fs\n", (tbuf2.tms_utime - tbuf1.tms_utime) / 60.0, (tbuf2.tms_stime - tbuf1.tms_stime) * 60);

		gcrDumpResult(ch[i], GcrShowEnd, outfp);
		//gcrShowMap(ch);
	}
	(void) fclose(outfp);
	return (ch);
}
#endif

void freeChannel(ch)
	GCRChannel *ch;
{
	GCRNet *net = NULL;
	int i = -1;
	for (net = ch->gcr_nets; net; net = net->gcr_next)
	{
		for (i = 0; i < ch->gcr_length + 2; i++)
		{
			freeMagic((void *)net->path[i]);
		}
		freeMagic((void *)net->path);
		freeMagic((char *)net);
	}

	ch->gcr_nets = NULL;

	freeMagic((void *)ch->gcr_lCol);
	freeMagic((void *)ch);

	ch->gcr_lCol = NULL;
	ch = NULL;
}

void
GCRRouteFromFile(fname)
    char *fname;
{
    static Point initOrigin = { 0, 0 };
    struct tms tbuf1, tbuf2;
    GCRChannel **ch;
    Transform trans;
    FILE *fp, *fp1, *outfp;
    Rect box;
    int j, chan_num, max_nets_cnt, tot_nets_cnt, num_route_error;
	int max_density, tot_density, failed_chans;
	char c;

    fp = fopen(fname, "r");
    if (fp == NULL)
    {
		perror(fname);
		return;
    }
	outfp = fopen("gcr_out.txt", "w");
	if(!outfp) printf("Error: open gcr_out.txt for write failed\n");

	c = getc(fp);
    if (c != '*')
    {
		printf("Old-style channel format no longer supported.\n");
		fprintf(outfp, "Old-style channel format no longer supported.\n");
		return ;
    }
	if (fscanf(fp, "%d", &chan_num) != 1)
    {
		printf("Format error in input file channel number.\n");
		fprintf(outfp,"Format error in input file channel number.\n");
		return;
    }

	printf("The number of channel is %d\n", chan_num);
	fprintf(outfp, "The number of channel is %d\n",chan_num);
	
	ch = (GCRChannel **)mallocMagic(chan_num * (unsigned)(sizeof(GCRChannel *)));
	
	for(int i=0; i<chan_num; i++)
	{
		ch[i] = (GCRChannel *) mallocMagic((unsigned) (sizeof (GCRChannel)));
    	ch[i]->gcr_type = CHAN_NORMAL;
	/*     ch->gcr_area = box; */
    	ch[i]->gcr_transform = GeoIdentityTransform;
    	ch[i]->gcr_lCol = (GCRColEl *) NULL;
    	ch[i]->gcr_nets = (GCRNet *) NULL;
		ch[i]->net_cnt = 0;
		ch[i]->failed_net = 0;
    	ch[i]->gcr_result = (short **) NULL;
    	ch[i]->gcr_origin = initOrigin;

		//printf("channel[%d] \n",i+1);
		//fprintf(outfp,"channel[%d] \n",i+1);

		if (!gcrMakeChannel(ch[i], fp, outfp))
    	{
			printf("Couldn't initialize channel routing problem\n");
			fprintf(outfp,"Couldn't initialize channel routing problem\n");
			(void) fclose(fp);
			(void) fclose(outfp);
			freeMagic((char *)ch[i]);
			freeMagic((void *)ch);
			return;
    	}
	}

    (void) fclose(fp);

	num_route_error = 0;

	for (int i = 0; i < chan_num; i++)
	{
		printf("\n\nchannel[%d] \n",i+1);
		fprintf(outfp,"\n\nchannel[%d] \n",i +1);
		printf("chan width is %d, len is %d\n", ch[i]->gcr_width, ch[i]->gcr_length);
		fprintf(outfp, "chan width is %d, len is %d\n", ch[i]->gcr_width, ch[i]->gcr_length);

		ch[i]->gcr_lCol = (GCRColEl *)mallocMagic((unsigned)((ch[i]->gcr_width + 2) * sizeof(GCRColEl)));

		times(&tbuf1);

		num_route_error = GCRroute(ch[i], outfp);
 
		times(&tbuf2);

		printf("Time   :  %5.2fu  %5.2ld\n", (tbuf2.tms_utime - tbuf1.tms_utime) / 60.0, (tbuf2.tms_stime - tbuf1.tms_stime) * 60);
		fprintf(outfp,"Time   :  %5.2fu  %5.2ld\n", (tbuf2.tms_utime - tbuf1.tms_utime) / 60.0, (tbuf2.tms_stime - tbuf1.tms_stime) * 60);

		gcrDumpResult(ch[i], GcrShowEnd, outfp); //print result array

		DumpNetPath(ch[i], outfp);//print path array
		//gcrShowMap(ch);
	}
	(void) fclose(outfp);

	//print jtl routing statistics
	fp1 = fopen("sbox_detail.txt", "w");
    	if(!fp1) printf("Error: open sbox_detail.txt for write failed\n");

    fprintf(fp1, "ch_x ch_y max_den ave_den nets failed\n");
	max_nets_cnt = 0;
	tot_nets_cnt = 0;
	failed_chans = 0;
	for (int i = 0; i < chan_num; i++)
	{
		if (ch[i]->net_cnt > max_nets_cnt)
		{
			max_nets_cnt = ch[i]->net_cnt;
		}
		tot_nets_cnt += ch[i]->net_cnt;
		fprintf(fp1, "%d %d ", ch[i]->gcr_origin.p_x, ch[i]->gcr_origin.p_y);

		max_density = 0;
		tot_density = 0;
		for (int j = 0; j < ch[i]->gcr_length + 2; j++)
		{
			max_density = ch[i]->gcr_density[j] > max_density ? ch[i]->gcr_density[j] : max_density;
			tot_density += ch[i]->gcr_density[j];
		}
		fprintf(fp1, "%d %f %d %d\n", max_density, (float)tot_density / (float)(ch[i]->gcr_length + 2), ch[i]->net_cnt, ch[i]->failed_net);
		if (ch[i]->failed_net > 0)
		{
			failed_chans++;
		}
	}
	fclose(fp1);
	printf("max_nets_cnt=%d\n", max_nets_cnt);
	printf("ave_nets_cnt_per_chan=%f\n", (float)tot_nets_cnt / (float)chan_num);
	printf("failed_chans=%d\n", failed_chans);

	if (num_route_error > 0)
		printf("Error: JTL routing failed\n");

	for (int i = 0; i < chan_num; i++)
	{
		freeChannel(ch[i]);
	}
	freeMagic((char *)ch);
}


bool get_net_id(char *s, int *pid){
	int len;
	char dst[25] = "";
    
    if(s[0] == '0') {
		*pid = 0;
		return FALSE;
	}	

	len = strlen(s) - 1; //remove d(river) or r(eceiver) char
	
    if(s[len] != 'd' && s[len] != 'r'){
		printf("Error: %s in net id in input file, len is %d\n", s, len);
		return -1;
	}
	strncpy(dst, s, len);
	*pid = atoi(dst);
	
	return s[len] == 'd';

}



/*
 * ----------------------------------------------------------------------------
 *
 * gcrMakeChannel --
 *
 * 	Read a channel in the new file format.
 *
 * Results:
 *	TRUE if successful, FALSE if not.
 *
 * Side effects:
 *	Sets values in *channel.
 *
 * ----------------------------------------------------------------------------
 */

bool
gcrMakeChannel(ch, fp, outfp)
    GCRChannel *ch;
    FILE *fp;
	FILE *outfp;
{
    GCRPin *gcrMakePinLR();
    unsigned lenWds, widWds;
    int i, j, c = 0, pid;
    char s[25];

	c = getc(fp);
	while (c != '*')
	{
		c = getc(fp);
	}
	
    if (c != '*')
    {
		printf("Old-style channel format no longer supported.\n");
		fprintf(outfp,"Old-style channel format no longer supported.\n");
		return (FALSE);
    }
	if (fscanf(fp, "%d %d", &ch->gcr_origin.p_x, &ch->gcr_origin.p_y) != 2)
	{
		printf("Format error in input file channel origin.\n");
		fprintf(outfp, "Format error in input file channel origin.\n");
		return(FALSE);
	}
    if (fscanf(fp, "%d %d", &ch->gcr_width, &ch->gcr_length) != 2)
    {
		printf("Format error in input file width or length.\n");
		fprintf(outfp, "Format error in input file width or length.\n");
		return (FALSE);
    }

    lenWds = ch->gcr_length + 2;
    widWds = ch->gcr_width + 2;
    ch->gcr_density = (int *) mallocMagic((unsigned) (lenWds * sizeof (int)));

	//initialize left pin
    ch->gcr_lPins = gcrMakePinLR(fp, 0, ch->gcr_width);
    
    ch->gcr_tPins = (GCRPin *) mallocMagic((unsigned) (lenWds * sizeof (GCRPin)));
    ch->gcr_bPins = (GCRPin *) mallocMagic((unsigned) (lenWds * sizeof (GCRPin)));
    ch->gcr_result = (short **) mallocMagic((unsigned) (lenWds * sizeof (short *)));

    /* initialize end columns */
    ch->gcr_result[0] = (short *) mallocMagic((unsigned) (widWds * sizeof (short)));
    ch->gcr_result[ch->gcr_length+1] = (short *) mallocMagic((unsigned) (widWds * sizeof (short)));
    for (i = 0; i < widWds; i++)
    {
		ch->gcr_result[0][i] = 0;
		ch->gcr_result[ch->gcr_length + 1][i] = 0;
    }

    /* initialize internal columns */
    for (i = 1; i <= ch->gcr_length; i++)
    {
		/* allocate the column */
		ch->gcr_result[i] = (short *) mallocMagic((unsigned) (widWds * sizeof (short)));

		/* Initialize the bottom pin */
		if (fscanf(fp, "%s", s) != 1){
	    	printf("Format error in net-id in channel file, d(river) or r(eceiver) should be annotated\n");
			fprintf(outfp, "Format error in net-id in channel file, d(river) or r(eceiver) should be annotated\n");
	    	return (FALSE);
		
		}
	    ch->gcr_bPins[i].isSrc = get_net_id(s, &pid);
		
		//if (fscanf(fp, "%d", &pid) != 1)
		//{
	    //	printf("Format error in pin-id in channel file\n");
	    //	return (FALSE);
		//}	
		//ch->gcr_bPins[i].gcr_pId = (GCRNet *)(spointertype) pid;
		ch->gcr_bPins[i].gcr_pSeg = pid;
		ch->gcr_bPins[i].gcr_x = i;
		ch->gcr_bPins[i].gcr_y = 0;

		ch->gcr_result[i][0] = 0;
		ch->gcr_result[i][ch->gcr_width+1] = 0;
		for (j = 1; j <= ch->gcr_width; j++)
		{
	    /*
	     * Read a column of obstacles.  m and M mean metal is blocked,
	     * p and P mean poly is blocked.  Upper case means vacate the
	     * column, lower case means vacate the track.
	     */
	    	if (fscanf(fp, "%s", s) != 1)
	    	{
				printf("Format error in router input file\n");
				fprintf(outfp, "Format error in router input file\n");
				return (FALSE);
	    	}
			//printf("read str %s\n", s);
	    	switch (s[0])
	    	{
			case 'M': case 'm':
		    	ch->gcr_result[i][j] = GCRBLKM;
		    	break;
			case 'P': case 'p':
		    	ch->gcr_result[i][j] = GCRBLKP;
		    	break;
			case '.':
		    	ch->gcr_result[i][j] = 0;
		    	break;
			default:
		    	ch->gcr_result[i][j] = GCRBLKP | GCRBLKM;
		    	break;
	    	}
		}

		/* Read top pin id */
		if (fscanf(fp, "%s", s) != 1){
	    	printf("Format error in net-id in channel file, d(river) or r(eceiver) should be annotated\n");
			fprintf(outfp, "Format error in net-id in channel file, d(river) or r(eceiver) should be annotated\n");
	    	return (FALSE);
		
		}
	   ch->gcr_tPins[i].isSrc = get_net_id(s, &pid);
	#if 0
		if (fscanf(fp, "%d", &pid) != 1)

		{
	    	printf("Format error in router input file\n");
	    	return (FALSE);
		}
	#endif
		//ch->gcr_tPins[i].gcr_pId = (GCRNet *)(spointertype) pid;
		ch->gcr_tPins[i].gcr_pSeg = pid;
  		//printf("top netid %d\n", ch->gcr_tPins[i].gcr_pId->gcr_Id);
		ch->gcr_tPins[i].gcr_x = i;
		ch->gcr_tPins[i].gcr_y = ch->gcr_width + 1;
    }

    ch->gcr_rPins = gcrMakePinLR(fp,ch->gcr_length + 1, ch->gcr_width);

	ch->gcr_area.r_xbot = 0;
    ch->gcr_area.r_ybot = 0;
    ch->gcr_area.r_xtop = (ch->gcr_length + 1) * RtrGridSpacing;
    ch->gcr_area.r_ytop = (ch->gcr_width  + 1) * RtrGridSpacing;

    return (TRUE);
}

GCRPin *
gcrMakePinLR(fp, x, size)
    FILE *fp;
    int x, size;
{
    GCRPin *result;
    int i, pid;
    GCRNet* net;
	char s[25];

    result = (GCRPin *) mallocMagic((unsigned) ((size+2) * sizeof (GCRPin)));
    result[0].gcr_x = result[0].gcr_y = 0;
    result[0].gcr_pId = (GCRNet *) NULL;
    result[size + 1].gcr_x = result[size + 1].gcr_y = 0;
    result[size + 1].gcr_pId = (GCRNet *) NULL;
    for (i = 1; i <= size; i++)
    {
		/* FIXME: Reading a pointer from file is almost guaranteed to break. */
		//dlong pointer_bits;
		//	(void) fscanf(fp, "%"DLONG_PREFIX"d", &pointer_bits);
		//	result[i].gcr_pId = (struct gcrnet *) pointer_bits;
		fscanf(fp, "%s", s);
		pid = 0;
		result[i].isSrc = get_net_id(s, &pid);
		if(!result[i].isSrc && pid != 0)
			result[i].isSink == 1;
		result[i].gcr_pSeg = pid;

		result[i].gcr_x = x;
		result[i].gcr_y = i;
    }

    return (result);
}

/*
 * ----------------------------------------------------------------------------
 *
 * gcrSaveChannel --
 *
 * Write a channel file for subsequent use in debugging.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates a disk file named by the channel address,
 *	e.g, chan.1efb0
 *
 * ----------------------------------------------------------------------------
 */

void
gcrSaveChannel(ch)
    GCRChannel *ch;
{
    FILE *fp;
    char name[128];
    int flags, i, j;

    (void) sprintf(name, "chan.%p", ch);
    fp = fopen(name, "w");
    if (fp == NULL)
    {
	printf("Can't dump channel to file; "); fflush(stdout);//TxFlush();
	perror(name);
	return;
    }

    /* Output the prologue */
    fprintf(fp, "* %d %d\n", ch->gcr_width, ch->gcr_length);

#define	NETID(pin)	((pin).gcr_pId ? (pin).gcr_pId->gcr_Id : 0)

    /* Output the left pin array */
    for (j = 1; j <= ch->gcr_width; j++)
	fprintf(fp, "%d ", NETID(ch->gcr_lPins[j]));
    fprintf(fp, "\n");

    /* Process main body of channel */
    for (i = 1; i <= ch->gcr_length; i++)
    {
	/* Bottom pin */
	fprintf(fp, "%d ", NETID(ch->gcr_bPins[i]));

	/*
	 * Interior points (for obstacle map).
	 * Codes are as follows:
	 *
	 *	m	metal blocked		vacate track
	 *	M	metal blocked		vacate column
	 *	p	poly blocked		vacate track
	 *	P	poly blocked		vacate column
	 */
	for (j = 1; j <= ch->gcr_width; j++)
	{
	    flags = ch->gcr_result[i][j];
	    switch (flags & (GCRBLKM|GCRBLKP))
	    {
		case 0:			fprintf(fp, ". "); break;
		case GCRBLKM:		fprintf(fp, "m "); break;
		case GCRBLKP:		fprintf(fp, "p "); break;
		case GCRBLKM|GCRBLKP:	fprintf(fp, "x "); break;
	    }
	}

	/* Top pin */
	fprintf(fp, "%d\n", NETID(ch->gcr_tPins[i]));
    }

    /* Output the right pin array */
    for (j = 1; j <= ch->gcr_width; j++)
	fprintf(fp, "%d ", NETID(ch->gcr_rPins[j]));
    fprintf(fp, "\n");
    (void) fclose(fp);
}

/*
 * ----------------------------------------------------------------------------
 *
 * gcrPrDensity --
 *
 * Create a file of the form "dens.xbot.ybot.xtop.ytop", where
 * xbot, ybot are the lower-left coordinates of ch->gcr_area
 * and xtop, ytop are the upper right coordinates.  This file
 * contains a comparison of the density computed by the global
 * router with that computed by the channel router; it is used
 * for debugging only.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates a file named as described above.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrPrDensity(ch, chanDensity)
    GCRChannel *ch;
    int chanDensity;
{
    int i, diff;
    char name[256];
    FILE *fp;

    (void) sprintf(name, "dens.%d.%d.%d.%d",
		ch->gcr_area.r_xbot, ch->gcr_area.r_ybot,
		ch->gcr_area.r_xtop, ch->gcr_area.r_ytop);
    fp = fopen(name, "w");
    if (fp == NULL)
	fp = stdout;

    fprintf(fp, "Chan width: %d\n", ch->gcr_width);
    fprintf(fp, "Chan length: %d\n", ch->gcr_length);
    fprintf(fp, "Chan area: ll=(%d,%d) ur=(%d,%d)\n",
		ch->gcr_area.r_xbot, ch->gcr_area.r_ybot,
		ch->gcr_area.r_xtop, ch->gcr_area.r_ytop);
    fprintf(fp, "Max column density (global):  %d\n", ch->gcr_dMaxByCol);
    fprintf(fp, "Max column density (channel): %d\n", chanDensity);
    fprintf(fp, "Column density by column:\n");
    fprintf(fp, "%3s %5s", "COL", "GLOB");
#ifdef	IDENSITY
    fprintf(fp, " %5s %5s", "INIT", "DIFF");
#endif	/* IDENSITY */
    fprintf(fp, " %5s\n", "CHAN");
    for (i = 1; i <= ch->gcr_length; i++)
    {
	fprintf(fp, "%3d %5d", i, ch->gcr_dRowsByCol[i]);
	diff = ch->gcr_dRowsByCol[i];
#ifdef	IDENSITY
	diff -= ch->gcr_iRowsByCol[i];
	fprintf(fp, " %5d %5d", ch->gcr_iRowsByCol[i], diff);
#endif	/* IDENSITY */
	fprintf(fp, "%5d%s\n", ch->gcr_density[i],
		(diff != ch->gcr_density[i]) ? " *****" : "");
    }
    fprintf(fp, "------\n");
    fprintf(fp, "Row density by column (global only):\n");
    fprintf(fp, "%3s %5s", "ROW", "GLOB");
#ifdef	IDENSITY
    fprintf(fp, " %5s %5s", "INIT", "DIFF");
#endif	/* IDENSITY */
    fprintf(fp, "\n");
    for (i = 1; i <= ch->gcr_width; i++)
    {
	fprintf(fp, "%3d %5d", i, ch->gcr_dColsByRow[i]);
#ifdef	IDENSITY
	fprintf(fp, " %5d %5d", ch->gcr_iColsByRow[i],
			ch->gcr_dColsByRow[i] - ch->gcr_iColsByRow[i]);
#endif	/* IDENSITY */
	fprintf(fp, "\n");
    }

    (void) fflush(fp);
    if (fp != stdout)
	(void) fclose(fp);
}


/*
 * ----------------------------------------------------------------------------
 *
 * gcrDumpPins --
 *
 * Prints a pin array.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
#if 0
void
gcrDumpPins(ch)
    GCRChannel *ch;
{
    int      i;
    GCRPin * pinArray;

    pinArray=ch->gcr_lPins;
    printf("LEFT PINS\n");
    for(i=0; i<=ch->gcr_width; i++)
    {
		printf("Location [%d]=%d:  x=%d, y=%d, pNext=%p, pPrev=%p, id=%d\n",
			i, &pinArray[i], pinArray[i].gcr_x, pinArray[i].gcr_y,
			pinArray[i].gcr_pNext, pinArray[i].gcr_pPrev, pinArray[i].gcr_pId);
    }
    pinArray=ch->gcr_rPins;
    printf("RIGHT PINS\n");
    for(i=0; i<=ch->gcr_width; i++)
    {
		printf("Location [%d]=%d:  x=%d, y=%d, pNext=%p, pPrev=%p, id=%d\n",
			i, &pinArray[i], pinArray[i].gcr_x, pinArray[i].gcr_y,
			pinArray[i].gcr_pNext, pinArray[i].gcr_pPrev, pinArray[i].gcr_pId);
    }
    pinArray=ch->gcr_bPins;
    printf("BOTTOM PINS\n");
    for(i=0; i<=ch->gcr_length; i++)
    {
		printf("Location [%d]=%d:  x=%d, y=%d, pNext=%p, pPrev=%p, id=%d\n",
			i, &pinArray[i], pinArray[i].gcr_x, pinArray[i].gcr_y,
			pinArray[i].gcr_pNext, pinArray[i].gcr_pPrev, pinArray[i].gcr_pId);
    }
    pinArray=ch->gcr_tPins;
    printf("TOP PINS\n");
    for(i=0; i<=ch->gcr_length; i++)
    {
		printf("Location [%d]=%d:  x=%d, y=%d, pNext=%p, pPrev=%p, id=%d\n",
			i, &pinArray[i], pinArray[i].gcr_x, pinArray[i].gcr_y,
			pinArray[i].gcr_pNext, pinArray[i].gcr_pPrev, pinArray[i].gcr_pId);
    }
}
#endif


/*
 * ----------------------------------------------------------------------------
 *
 * gcrDumpPinList --
 *
 * Prints a list of pins for a net.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
#if 0
void
gcrDumpPinList(pin, dir)
    GCRPin *pin;
    bool dir;
{
    if (pin)
    {
	printf("Location (%d, %d)=%x:  pNext=%d, pPrev=%d, id=%d\n",
		pin->gcr_x, pin->gcr_y, pin,
		pin->gcr_pNext, pin->gcr_pPrev, pin->gcr_pId);
	if (dir) gcrDumpPinList(pin->gcr_pNext, dir);
	else gcrDumpPinList(pin->gcr_pPrev, dir);
    }
}
#endif


/*
 * ----------------------------------------------------------------------------
 *
 * gcrDumpCol --
 *
 * Print the left column contents.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrDumpCol(col, size)
    GCRColEl *col;
    int size;
{
    int i;

    if (!gcrStandalone)
	return;

    for (i = size; i >= 0; i--)
	printf("[%2d] hi=%6d(%c) lo=%6d(%c) f=%4d\n", i,
	    col[i].gcr_hi, col[i].gcr_hOk ? 'T' : 'F',
	    col[i].gcr_lo, col[i].gcr_lOk ? 'T' : 'F',
	   	col[i].gcr_flags);
#if 0
	printf("[%2d] hi=%6d(%c) lo=%6d(%c) w=%6d f=%4d\n", i,
	    col[i].gcr_hi, col[i].gcr_hOk ? 'T' : 'F',
	    col[i].gcr_lo, col[i].gcr_lOk ? 'T' : 'F',
	    //col[i].gcr_h, col[i].gcr_v,
	    col[i].gcr_wanted, col[i].gcr_flags);
#endif
}


void DumpNetPath(ch, fp)
	GCRChannel *ch;
	FILE *fp;
{
	GCRNet *net;
	int i,j;

    ASSERT(ch, "Error: ch is null");
	printf("ch %d %d\n", ch->gcr_origin.p_x, ch->gcr_origin.p_y);
    fprintf(fp, "ch %d %d\n", ch->gcr_origin.p_x, ch->gcr_origin.p_y);
	for (net = ch->gcr_nets; net; net = net->gcr_next){
	    ch->net_cnt++;
	    ASSERT(net, "Error: net is null");
		//fprintf(fp, "net %d, src(%d, %d), sink(%d, %d)\n", net->gcr_Id, net->driver.p_x, net->driver.p_y, net->receiver.p_x, net->receiver.p_y);
		fprintf(fp, "net %d\n", net->gcr_Id);
		printf("net %d\n", net->gcr_Id);
		for(i = 1; i <= ch->gcr_length; i++){
			for(j = 1; j <= ch->gcr_width; j++){
				if(net->path[i][j] != 0){
					fprintf(fp, "c %d r %d j %u\n", i, j, net->path[i][j]);
					printf("c %d r %d j %u\n", i, j, net->path[i][j]);
				}
			}		
		}
		fprintf(fp, "*\n");
		printf("*\n");
	
	}
	fprintf(fp, ".\n");
	printf(".\n");

}



/*
 * ----------------------------------------------------------------------------
 *
 * gcrDumpResult --
 *
 * Print the results of the routing, up to the current column
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrDumpResult(ch, showResult, outfp)
    GCRChannel *ch;
    bool showResult;
	FILE *outfp;
{
    int j;

    if(!showResult)
	return;

    gcrStats(ch, outfp);
    
#if 0
    printf("         ");
    for (j = 1; j <= ch->gcr_width; j++)
	if (ch->gcr_lPins[j].gcr_pId)
	    printf("dump left%2dB", ch->gcr_lPins[j].gcr_pId->gcr_Id);
	else
	    printf("  ");
    printf("\n");

    for (j = 0; j <= ch->gcr_length; j++)
	gcrPrintCol(ch, j, showResult);

    printf("         ");
    for (j = 1; j <= ch->gcr_width; j++)
	if (ch->gcr_rPins[j].gcr_pId)
	    printf("%2d", ch->gcr_rPins[j].gcr_pId->gcr_Id);
	else
	    printf("  ");
    printf("\n");
#endif
}

void  gcrPrintCol(ch, i, showResult)
    GCRChannel *ch;
    int i, showResult;
{
    short **res = ch->gcr_result;
    int j;

    if (!showResult)
	return;

    if (i>0)
    {
	if(ch->gcr_bPins[i].gcr_pId)
	    printf("[%3d] %2d:", i, (int) ch->gcr_bPins[i].gcr_pId->gcr_Id);
	else printf("[%3d]  0:", i);
	for (j = 0; j <= ch->gcr_width; j++)
	{
	    if (j != 0)
	    {
		if ((res[i][j] & GCRX) && (!(res[i][j] & (GCRBLKM|GCRBLKP))))
		{
		    printf("X");
		    gcrViaCount++;
		}
		else if ((res[i][j] & GCRR) || (i > 0 && (res[i-1][j] & GCRR)))
		{
		    if (res[i][j] & GCRBLKM) printf("|");
		    else if ((res[i][j] & GCRU)
			|| (j != 0 && (res[i][j-1] & GCRU)))
		    {
			if ((res[i][j]&GCRBLKM) && !(res[i][j]&GCRR))
			    printf("+");
			else if (res[i][j]&GCRBLKP)
			    printf("#");
			else
			    printf(")");
		    }
		    else printf("#");
		}
		else if ((res[i][j] & GCRU) || j != 0 && (res[i][j-1] & GCRU))
		{
		    if((res[i][j]&GCRCC) && (!(res[i][j]&(GCRBLKM|GCRBLKP))))
		    {
			gcrViaCount++;
			printf("X");
		    }
		    else
		    if(res[i][j] & GCRBLKP)
			printf("#");
		    else
		    if(res[i][j+1] & GCRBLKP)
			printf("#");
		    else
		    if(res[i][j] & GCRVM)
			printf("#");
		    else
			printf("-");
		}
		else
		if((res[i][j] & GCRBLKM) && (res[i][j] & GCRBLKP))
		    printf("~");
		else
		if(res[i][j] & GCRBLKM)
		    printf("'");
		else
		if(res[i][j] & GCRBLKP)
		    printf("`");
		else
	    printf(" ");
	    }

	    if(res[i][j] & GCRU)
		if(res[i][j] & GCRBLKP)
		    printf("#");
		else
		if(res[i][j+1] & GCRBLKP)
		    printf("#");
		else
		if(res[i][j] & GCRVM)
		    printf("#");
		else
		    printf("-");
	    else
	    if((res[i][j] & GCRBLKM) && (res[i][j] & GCRBLKP))
		printf("~");
	    else
	    if( ((res[i][j] & GCRBLKM) && (res[i][j+1] & GCRBLKP)) ||
	        ((res[i][j] & GCRBLKP) && (res[i][j+1] & GCRBLKM)) )
		printf("~");
	    else
	    if((res[i][j+1] & GCRBLKM) && (res[i][j+1] & GCRBLKP))
		printf("~");
	    else
	    if((res[i][j] & GCRBLKM) || (res[i][j+1] & GCRBLKM))
		printf("'");
	    else
	    if((res[i][j] & GCRBLKP) || (res[i][j+1] & GCRBLKP))
		printf("`");
	    else
		printf(" ");
	}
	if(ch->gcr_tPins[i].gcr_pId!=(GCRNet *) 0)
	    printf(":%2d {%2d}", (int) ch->gcr_tPins[i].gcr_pId->gcr_Id,
		    ch->gcr_density[i]);
	else
	    printf(":   {%2d}", ch->gcr_density[i]);
    }

    printf("\n        :");
    for(j=0; j<=ch->gcr_width; j++)
    {
	if(j!=0)
	{
	    if(res[i][j] & GCRR)
		if(res[i][j] & GCRBLKM)
		    printf("|");
		else
		if((i<=ch->gcr_length) && (res[i+1][j] & GCRBLKM))
		    printf("|");
		else
		    printf("#");
	    else
	    if(((res[i][j] & GCRBLKM)&&(res[i][j] & GCRBLKP)) ||
	       ((res[i+1][j] & GCRBLKM)&&(res[i+1][j] & GCRBLKP)))
		printf("~");
	    else
	    if((res[i][j] & GCRBLKM) || (res[i+1][j] & GCRBLKM))
		printf("'");
	    else
	    if((res[i][j] & GCRBLKP) || (res[i+1][j] & GCRBLKP))
		printf("`");
	    else
		printf(" ");
	}
	else
	if((j!=0) && (i!=0))
	{
	    if(res[i-1][j] & GCRR)
		if(res[i-1][j] & GCRBLKM)
		    printf("|");
		else
		if((i<ch->gcr_length) && (res[i][j] & GCRBLKM))
		    printf("|");
		else
		    printf("#");
	    else
	    if(((res[i-1][j] & GCRBLKM)&&(res[i-1][j] & GCRBLKP)) ||
	       ((res[i  ][j] & GCRBLKM)&&(res[i  ][j] & GCRBLKP)))
		printf("~");
	    else
	    if((res[i-1][j] & GCRBLKM) || (res[i][j] & GCRBLKM))
		printf("'");
	    else
	    if((res[i-1][j] & GCRBLKP) || (res[i][j] & GCRBLKP))
		printf("`");
	    else
		printf(" ");
	}

	if((((res[i][j] & GCRBLKM) && (res[i][j] & GCRBLKP)) ||
	    ((res[i][j+1] & GCRBLKM) && (res[i][j+1] & GCRBLKP))) ||
	   (((res[i+1][j] & GCRBLKM) && (res[i+1][j] & GCRBLKP)) ||
	    ((res[i+1][j+1] & GCRBLKM) && (res[i+1][j+1] & GCRBLKP))))
	    printf("~");
	else
	if(((res[i][j] & GCRBLKM) || (res[i][j+1] & GCRBLKM))||
	   ((res[i+1][j] & GCRBLKM) || (res[i+1][j+1] & GCRBLKM)))
	    printf("'");
	else
	if(((res[i][j] & GCRBLKP) || (res[i][j+1] & GCRBLKP))||
	   ((res[i+1][j] & GCRBLKP) || (res[i+1][j+1] & GCRBLKP)))
	    printf("`");
	else
	    printf(" ");
    }
    printf(":\n");
}

/*
 * ----------------------------------------------------------------------------
 *
 * gcrStats --
 *
 * 	Print routing statistics.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrStats(ch, outfp)
    GCRChannel * ch;
	FILE *outfp;
{
    int wireLength=0, viaCount=0, row, col;
    short **res, mask, code, code2;
    int hWire=0, vWire=0;

    viaCount=0;
    res=ch->gcr_result;
    printf("ch_len %d, width %d\n", ch->gcr_length, ch->gcr_width);
	fprintf(outfp, "ch_len %d, width %d\n", ch->gcr_length, ch->gcr_width);
    for(col = 0; col <= ch->gcr_length; col++)
	for(row = 0; row <= ch->gcr_width; row++)
	{
        printf("\n col:%d  row:%d ", col, row);
		fprintf(outfp, "\n col:%d  row:%d ", col, row);
	    code = res[col][row];
	    if(code & GCRR)
	    {
			wireLength++;
			hWire++;
 			printf("h  ");
			fprintf(outfp, "h  ");
	    }
	    if(code & GCRU)
	    {
			wireLength++;
			vWire++;
 			printf("v  ");
			fprintf(outfp, "v  ");
	    }
	    if(code & GCRX)
	    {
	    /* There is a connection at the crossing.  Count a contact if metal
	     * and poly come together here.
	     */
			mask = 0;
			code2 = ch->gcr_result[col][row+1];
			if(code & GCRU)
			{
		    	if(code & GCRVM) mask |= GCRBLKM;	/*What type is up*/
		   		else mask |= GCRBLKP;
			}

			code2 = ch->gcr_result[col+1][row];
			if(code & GCRR)
			{
		    	if(code2 & GCRBLKM) mask |= GCRBLKP;	/*What type is right*/
			    else mask |= GCRBLKM;
			}

			code2 = ch->gcr_result[col][row-1];
		    if(code2 & GCRU)
			{
		    	if(code2&GCRVM) mask|=GCRBLKM;	/*What type is down*/
		        else mask|=GCRBLKP;
			}

			code2 = ch->gcr_result[col-1][row];
			if(code2 & GCRR)
			{
		    	if(code2 & GCRBLKM) mask |= GCRBLKP;	/*What type is left*/
			    else mask |= GCRBLKM;
			}
			if((mask != GCRBLKM) && (mask != GCRBLKP)){
		    	viaCount++;
 		    	printf("via  ");
				fprintf(outfp, "via  ");
			}

	    }
	}
    printf("\n\nLength :  %d\n", wireLength);
    printf("Vias   :  %d\n", viaCount);
    printf("Hwire  :  %d\n", hWire);
    printf("Vwire  :  %d\n", vWire);
	fprintf(outfp, "\n\nLength :  %d\n", wireLength);
    fprintf(outfp, "Vias   :  %d\n", viaCount);
    fprintf(outfp, "Hwire  :  %d\n", hWire);
    fprintf(outfp, "Vwire  :  %d\n", vWire);
}

/*
 * ----------------------------------------------------------------------------
 *
 * gcrCheckCol --
 *
 * Check the accuracy of the column's hi and lo pointers.  Abort if there
 * is a mistake.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrCheckCol(ch, c, where)
    GCRChannel *ch;
    int c;
    char *where;
{
    int i, j;
    GCRColEl * col;

    if(GcrNoCheck)
	return;
    col=ch->gcr_lCol;
    for(i=0; i<=ch->gcr_width; i++)
    {
	if( (col[i].gcr_hOk || col[i].gcr_lOk) && (col[i].gcr_h==(GCRNet *) 0))
	{
	    if(gcrStandalone)
	    {
	    printf("Botch at column %d, %s (bad hOk/lOk at %d)\n",
	        c, where, i);
	    gcrDumpCol(col, ch->gcr_width);
	    }
	    if(GcrDebug)
	       abort();
	}
	if(((col[i].gcr_hi==i)||(col[i].gcr_lo==i))&&(i!=0))
	{
	    if(gcrStandalone)
	    {
	    printf("Botch at column %d, %s(pointer loop at %d)\n",
		c, where, i);
	    gcrDumpCol(col, ch->gcr_width);
	    }
	    if(GcrDebug)
	       abort();
	}
	if(col[i].gcr_h!=(GCRNet *) NULL)
	
	/* Look upward from the track for the next higher track assigned to
	 * the net, if any.  Just take the first one, breaking afterwards.
	 */
	    for(j=i+1; j<=ch->gcr_width; j++)
	    {
		if(col[j].gcr_h==col[i].gcr_h)
		{
		/* Check to see if the lower track at i points to the higher
		 * track at j, and vice versa.  If an error, abort.
		 */
		    if( ((col[j].gcr_lo!=i) && !col[j].gcr_lOk &&
			!col[i].gcr_hOk) ||
		        ((col[i].gcr_hi!=j) && !col[i].gcr_hOk &&
			!col[j].gcr_lOk) )
		    {
			if(gcrStandalone)
			{
			printf("Botch at column %d, %s", c, where);
			printf(" (link error from %d to %d)\n", i, j);
			gcrDumpCol(col, ch->gcr_width);
			}
			if(GcrDebug) abort();
		    }
		    else break;
		}
	    }
	if((col[i].gcr_hi>ch->gcr_width)||(col[i].gcr_hi< EMPTY)||
	   (col[i].gcr_lo>ch->gcr_width)||(col[i].gcr_lo< EMPTY))
	{
	    if(gcrStandalone)
	    {
	    printf("Botch at column %d, %s (bounds)\n", c, where);
	    gcrDumpCol(col, ch->gcr_width);
	    }
	    if(GcrDebug) abort();
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * gcrShowMap --
 *
 * Print the bit map in the result array for the selected field.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

void
gcrShowMap(ch)
    GCRChannel * ch;
{
    int i, j, field;
    short ** res;
    char buff[512];

    if (!GcrShowMap)
	return;

    while (1)
    {
	printf("Field selector (0 terminates): ");
	if(!scanf("%d", &field))	/*typed something funny*/
	{
	    printf("Bad input.  Legal responses are\n");
	    printf("   GCRBLKM     1\n");
	    printf("   GCRBLKP     2\n");
	    printf("   GCRU        4\n");
	    printf("   GCRR        8\n");
	    printf("   GCRX        16\n");
	    printf("   GCRVL       32\n");
	    printf("   GCRV2       64\n");
	    printf("   GCRTC       128\n");
	    printf("   GCRCC       256\n");
	    printf("   GCRTE       512\n");
	    printf("   GCRCE       1024\n");
	    printf("   GCRVM       2048\n");
	    printf("   GCRXX       4096\n");
	    printf("   GCRVR       8192\n");
	    printf("   GCRVU      16384\n");
	    printf("   GCRVD      32768\n");
	    (void) fgets(buff, 512, stdin);
	}
	printf("\n%d\n", field);
	if(field==0)
	    return;

	printf("\n     ");
	for(j=0; j<=ch->gcr_width+1; j++)
	    printf("%2d", j);

	for(i=0; i<=ch->gcr_length+1; i++)
	{
	    res=ch->gcr_result;
	    printf("\n[%3d] ", i);
	    for(j=0; j<=ch->gcr_width+1; j++)
	    {
		if(res[i][j] & field)
		    printf("1 ");
		else
		    printf(". ");
	    }
	}
	printf("\n");
    }
}