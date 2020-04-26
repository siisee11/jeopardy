#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/kref.h>
#include <linux/net.h>
#include <linux/export.h>
#include <net/tcp.h>

#include <asm/uaccess.h>

#include "heartbeat.h"
#include "tcp.h"
#include "nodemanager.h"
#define MLOG_MASK_PREFIX ML_TCP
#include "masklog.h"
#include "quorum.h"

#include "tcp_internal.h"

#define SC_NODEF_FMT "node %s (num %u) at %pI4:%u"
#define SC_NODEF_ARGS(sc) sc->sc_node->nd_name, sc->sc_node->nd_num,	\
	&sc->sc_node->nd_ipv4_address,		\
	ntohs(sc->sc_node->nd_ipv4_port)


static struct pmdfc_sock_container *sc_alloc()
{
	struct pmdfc_sock_container *sc, *ret = NULL;
	struct page *page = NULL;
	int status = 0;

	page = alloc_page(GFP_NOFS);
	sc = kzalloc(sizeof(*sc), GFP_NOFS);
	if (sc == NULL || page == NULL)
		goto out;

	kref_init(&sc->sc_kref);
	o2nm_node_get(node);
	sc->sc_node = node;

	/* pin the node item of the remote node */
	status = o2nm_depend_item(&node->nd_item);
	if (status) {
		mlog_errno(status);
		o2nm_node_put(node);
		goto out;
	}
	INIT_WORK(&sc->sc_connect_work, o2net_sc_connect_completed);
	INIT_WORK(&sc->sc_rx_work, o2net_rx_until_empty);
	INIT_WORK(&sc->sc_shutdown_work, o2net_shutdown_sc);
	INIT_DELAYED_WORK(&sc->sc_keepalive_work, o2net_sc_send_keep_req);

	init_timer(&sc->sc_idle_timeout);
	sc->sc_idle_timeout.function = o2net_idle_timer;
	sc->sc_idle_timeout.data = (unsigned long)sc;

	ret = sc;
	sc->sc_page = page;
	o2net_debug_add_sc(sc);
	sc = NULL;
	page = NULL;

out:
	if (page)
		__free_page(page);
	kfree(sc);

	return ret;
}
