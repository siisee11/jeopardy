// SPDX-License-Identifier: GPL-2.0-or-later
/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * Copyright (C) 2020 JY N.  All rights reserved.
 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/configfs.h>

#include "tcp.h"
#include "nodemanager.h"

struct pmnm_cluster *pmnm_single_cluster = NULL;

struct pmnm_node *pmnm_get_node_by_num(u8 node_num)
{
	struct pmnm_node *node = NULL;

	if (node_num >= PMNM_MAX_NODES || pmnm_single_cluster == NULL)
		goto out;

	read_lock(&pmnm_single_cluster->cl_nodes_lock);
	node = pmnm_single_cluster->cl_nodes[node_num];
	read_unlock(&pmnm_single_cluster->cl_nodes_lock);
out:
	return node;
}
EXPORT_SYMBOL_GPL(pmnm_get_node_by_num);

int init_pmnm_node(struct pmnm_node *node, 
		const char *name, char *ip, unsigned int port, int num){

	if (strlen(name) > PMNM_MAX_NAME_LEN)
		return ERR_PTR(-ENAMETOOLONG);

	node = kzalloc(sizeof(struct pmnm_node), GFP_KERNEL);
	if (node == NULL)
		return ERR_PTR(-ENOMEM);

	strcpy(node->nd_name, name); /* use item.ci_namebuf instead? */
	
	node->nd_num = num;
	node->nd_ipv4_address = inet_addr(ip);
	node->nd_ipv4_port = htons(port);

	printk(KERN_NOTICE "init_pmnm_node: "
			SC_NODEF_FMT "\n", node->nd_name,
			node->nd_num, node->nd_ipv4_address,
			ntohs(node->nd_ipv4_port));

	return 0;
}

static void pmnm_cluster_release(void)
{
	kfree(pmnm_single_cluster);
}

void init_pmnm_cluster(void){
	struct pmnm_cluster *cluster = NULL;
	int ret;
	cluster = kzalloc(sizeof(struct pmnm_cluster), GFP_KERNEL);

//	struct pmnm_node my_node = NULL;
	struct pmnm_node *target_node = NULL;

	pr_info("nodemanager.c::init_pmnm_cluster\n");

	ret = init_pmnm_node(target_node, "pm_server", DEST_ADDR, PORT, 0);
	if (ret < 0) {
		pr_info("init_pmnm_cluster failed\n");
		return;
	}

	cluster->cl_nodes[0] = target_node;
//	cluster->cl_nodes[1] = my_node;

	rwlock_init(&cluster->cl_nodes_lock);
	pmnm_single_cluster = cluster;
}

void exit_pmnm_cluster(void){
	pmnm_cluster_release();
}
