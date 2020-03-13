#include <linux/module.h> // included for all kernel modules
#include <linux/kernel.h> // included for KERN_INFO
#include <linux/init.h> // included for __init and __exit macros
#include <linux/skbuff.h> // included for struct sk_buff
#include <linux/if_packet.h> // include for packet info
#include <linux/ip.h> // include for ip_hdr
#include <linux/netdevice.h> // include for dev_add/remove_pack
#include <linux/if_ether.h> // include for ETH_P_ALL

MODULE_LICENSE("GPL");

struct packet_type ji_proto;

int ji_packet_rcv (struct sk_buff *skb, struct net_device *dev,
		struct packet_type *pt, struct net_device *orig_dev)
{
	struct ethhdr *eth;
	struct iphdr *ip_header;

	printk(KERN_INFO "@JI : New packet captured.\n");

	/* linux/if_packet.h : Packet types */
	// #define PACKET_HOST 0 /* To us */
	// #define PACKET_BROADCAST 1 /* To all */
	// #define PACKET_MULTICAST 2 /* To group */
	// #define PACKET_OTHERHOST 3 /* To someone else */
	// #define PACKET_OUTGOING 4 /* Outgoing of any type */
	// #define PACKET_LOOPBACK 5 /* MC/BRD frame looped back */
	// #define PACKET_USER 6 /* To user space */
	// #define PACKET_KERNEL 7 /* To kernel space */
	/* Unused, PACKET_FASTROUTE and PACKET_LOOPBACK are invisible to user space */
	// #define PACKET_FASTROUTE 6 /* Fastrouted frame */

	switch (skb->pkt_type)
	{
		case PACKET_HOST:
			printk(KERN_INFO "@JI : PACKET to us ");
			break;
		case PACKET_BROADCAST:
			printk(KERN_INFO "@JI : PACKET to all ");
			break;
		case PACKET_MULTICAST:
			printk(KERN_INFO "@JI : PACKET to group ");
			break;
		case PACKET_OTHERHOST:
			printk(KERN_INFO "@JI : PACKET to someone else ");
			break;
		case PACKET_OUTGOING:
			printk(KERN_INFO "@JI : PACKET outgoing ");
			break;
		case PACKET_LOOPBACK:
			printk(KERN_INFO "@JI : PACKET LOOPBACK ");
			break;
		case PACKET_FASTROUTE:
			printk(KERN_INFO "@JI : PACKET FASTROUTE ");
			break;
	}

	printk(KERN_CONT "Dev: %s ; 0x%.4X ; 0x%.4X \n", skb->dev->name,
			ntohs(skb->protocol), ip_hdr(skb)->protocol);

	eth = (struct ethhdr*)skb_mac_header(skb);
	ip_header = (struct iphdr *)skb_network_header(skb);
	printk("src mac %pM, dst mac %pM\n", eth->h_source, eth->h_dest);
	printk("src IP addr %pI4\n", &ip_header->saddr);


	kfree_skb (skb);
	return 0;
}

static int __init ji_init(void)
{
	/* See the <linux/if_ether.h>
	   When protocol is set to htons(ETH_P_ALL), then all protocols are received.
	   All incoming packets of that protocol type will be passed to the packet
	   socket before they are passed to the protocols implemented in the kernel. */
	/* Few examples */
	//ETH_P_LOOP 0x0060 /* Ethernet Loopback packet */
	//ETH_P_IP 0x0800 /* Internet Protocol packet */
	//ETH_P_ARP 0x0806 /* Address Resolution packet */
	//ETH_P_LOOPBACK 0x9000 /* Ethernet loopback packet, per IEEE 802.3 */
	//ETH_P_ALL 0x0003 /* Every packet (be careful!!!) */
	//ETH_P_802_2 0x0004 /* 802.2 frames */
	//ETH_P_SNAP 0x0005 /* Internal only */

	ji_proto.type = htons(ETH_P_IP);

	/* NULL is a wildcard */
	//ji_proto.dev = NULL;
	ji_proto.dev = dev_get_by_name (&init_net, "enp0s3");

	ji_proto.func = ji_packet_rcv;

	/* Packet sockets are used to receive or send raw packets at the device
	   driver (OSI Layer 2) level. They allow the user to implement
	   protocol modules in user space on top of the physical layer. */

	/* Add a protocol handler to the networking stack.
	   The passed packet_type is linked into kernel lists and may not be freed until
	   it has been removed from the kernel lists. */
	dev_add_pack (&ji_proto);

	printk(KERN_INFO "@JI : Module insertion completed successfully!\n");
	return 0; // Non-zero return means that the module couldn't be loaded.
}

static void __exit ji_cleanup(void)
{
	dev_remove_pack(&ji_proto);
	printk(KERN_INFO "@JI : Cleaning up module....\n");
}

module_init(ji_init);
module_exit(ji_cleanup);
