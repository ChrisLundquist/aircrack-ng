 /*
  *  Copyright (c) 2009, Kyle Fuller <inbox@kylefuller.co.uk>, based upon
  *  freebsd.c by Andrea Bittau <a.bittau@cs.ucl.ac.uk>
  *
  *  OS dependent API for Darwin.
  *
  *  This program is free software; you can redistribute it and/or modify
  *  it under the terms of the GNU General Public License as published by
  *  the Free Software Foundation; either version 2 of the License, or
  *  (at your option) any later version.
  *
  *  This program is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU General Public License for more details.
  *
  *  You should have received a copy of the GNU General Public License
  *  along with this program; if not, write to the Free Software
  *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pcap/pcap.h>
#include <strings.h> // memcpy
#include <sys/param.h> // MIN

#include "osdep.h"

int darwin_read(struct wif *wi, unsigned char *h80211, int len, struct rx_info *ri) {
    pcap_t *handle = NULL;/* Session handle */
    char *dev;          /* The device to sniff on */
    char errbuf[PCAP_ERRBUF_SIZE];  /* Error string */
    struct pcap_pkthdr header;  /* The header that pcap gives us */
    const u_char *packet;       /* The actual packet */
    bpf_u_int32 mask;       /* Our netmask */
    bpf_u_int32 net;        /* Our IP */

    // Make sure they don't read garbage from last time
    memset( h80211, 0, len);

    /* Define the device */
    dev = pcap_lookupdev(errbuf);
    if (dev == NULL) {
        fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
        return(-1);
    }
    /* Find the properties for the device */
    if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
        fprintf(stderr, "Couldn't get netmask for device %s: %s\n", dev, errbuf);
        net = 0;
        mask = 0;
    }

    /* Open the session in promiscuous mode */
    handle = pcap_create(dev, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return(-1);
    }
    if (!pcap_can_set_rfmon(handle)) {
        fprintf(stderr, "Monitor mode is not supported on this device\n");
        return -1;
    }

    if (pcap_set_rfmon(handle, 1)) {
        fprintf(stderr, "Failed to set monitor mode on this device\n");
        return -1;
    }
    pcap_set_snaplen(handle, len);
    pcap_set_promisc(handle, 1); // Turn promiscuous mode on
    pcap_set_timeout(handle, 512); // Set the timeout to 512 milliseconds
    if(pcap_activate(handle)) {
        pcap_perror(handle, "%s\n");
        return -1;
    }
    /* Grab a packet */
    packet = pcap_next(handle, &header);

    int min_len = 0;
    if (packet) {
        /* Print its length */
        printf("Jacked a packet with length of [%d]\n", header.len);
        for(int i = 0; i < header.len; i++) {
            printf("%02X", packet[i]);

        }
        printf("\n");
        min_len = MIN(len,header.len); // drop the MAC header

        if(h80211 && packet)
            memcpy(h80211, packet, min_len); // drop the MAC header
    }

    /* And close the session */
    pcap_close(handle);

    return min_len;
}

int darwin_write(struct wif *wi, unsigned char *h80211, int len, struct tx_info *ti) {
    pcap_t *handle;         /* Session handle */
    char *dev;          /* The device to sniff on */
    char errbuf[PCAP_ERRBUF_SIZE];  /* Error string */

    dev = pcap_lookupdev(errbuf);
    if (dev == NULL) {
        fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
        return(2);
    }

    /* Open the session */
    handle = pcap_open_live(dev, BUFSIZ, 0, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return(2);
    }

     // Write the Ethernet frame to the interface.
    if (pcap_inject(handle , h80211, len) < 0) {
        fprintf(stderr, "failed injecting packet: %s\n", errbuf);
        pcap_perror(handle, 0);
    }
    pcap_close(handle);

 return 0;
}

int darwin_set_ht_channel(struct wif *wi, int chan, unsigned int htval) {
 return 0;
}

int darwin_set_channel(struct wif *wi, int chan) {
 return 0;
}

int darwin_get_channel(struct wif *wi) {
 return 0;
}

int darwin_set_freq(struct wif *wi, int freq) {
 return 0;
}

int darwin_get_freq(struct wif *wi) {
 return 0;
}

void darwin_close(struct wif *wi) {
 return;
}

int	darwin_fd(struct wif *wi) {
 return 0;
}

int	darwin_get_mac(struct wif *wi, unsigned char *mac) {
 return 0;
}

int	darwin_set_mac(struct wif *wi, unsigned char *mac) {
 return 0;
}

int	darwin_set_rate(struct wif *wi, int rate) {
 return 0;
}

int	darwin_get_rate(struct wif *wi) {
 return 0;
}

int	darwin_set_mtu(struct wif *wi, int mtu) {
 return 0;
}

int	darwin_get_mtu(struct wif *wi) {
 return 0;
}

int darwin_get_monitor(struct wif *wi) {
 return 0;
}

struct wif *wi_open_osdep(char *ifname)
{
	if (ifname) {} /* XXX unused parameter */
    printf("I am on darwin, trying to open the wifi interface\n");

    struct wif* wi = calloc(1, sizeof(struct wif));
    wi->wi_read         = darwin_read;
    wi->wi_write        = darwin_write;
    wi->wi_set_channel  = darwin_set_channel;
    wi->wi_get_channel  = darwin_get_channel;
    wi->wi_set_freq		= darwin_set_freq;
    wi->wi_get_freq		= darwin_get_freq;
    wi->wi_close        = darwin_close;
    wi->wi_fd		    = darwin_fd;
    wi->wi_get_mac		= darwin_get_mac;
    wi->wi_set_mac		= darwin_set_mac;
    wi->wi_get_monitor  = darwin_get_monitor;
    wi->wi_get_rate		= darwin_get_rate;
    wi->wi_set_rate		= darwin_set_rate;
    wi->wi_get_mtu		= darwin_get_mtu;
    wi->wi_set_mtu		= darwin_set_mtu;
	//errno = EOPNOTSUPP;
	return wi;
}

int get_battery_state(void)
{
	errno = EOPNOTSUPP;
	return -1;
}

int create_tap(void)
{
	errno = EOPNOTSUPP;
	return -1;
}
