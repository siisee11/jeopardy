struct pmdfc_sock_container {
	struct kref			sc_kref;
	/* the next two are valid for the life time of the sc */
	struct socket		*sc_sock;

#if 0
	/* all of these sc work structs hold refs on the sc while they are
	 ** queued.  they should not be able to ref a freed sc.  the teardown
	 ** race is with o2net_wq destruction in o2net_stop_listening() */

	/* rx and connect work are generated from socket callbacks.  sc
	 ** shutdown removes the callbacks and then flushes the work queue */
	struct work_struct	sc_rx_work;
	struct work_struct	sc_connect_work;
	/* shutdown work is triggered in two ways.  the simple way is
	 ** for a code path calls ensure_shutdown which gets a lock, removes
	 ** the sc from the nn, and queues the work.  in this case the
	 ** work is single-shot.  the work is also queued from a sock
	 ** callback, though, and in this case the work will find the sc
	 ** still on the nn and will call ensure_shutdown itself.. this
	 ** ends up triggering the shutdown work again, though nothing
	 ** will be done in that second iteration.  so work queue teardown
	 ** has to be careful to remove the sc from the nn before waiting
	 ** on the work queue so that the shutdown work doesn't remove the
	 ** sc and rearm itself.
	 **/
	struct work_struct	sc_shutdown_work;

	struct timer_list	sc_idle_timeout;
	struct delayed_work	sc_keepalive_work;

	unsigned		sc_handshake_ok:1;

	struct page 		*sc_page;
	size_t			sc_page_off;

	/* original handlers for the sockets */
	void			(*sc_state_change)(struct sock *sk);
	void			(*sc_data_ready)(struct sock *sk);

	u32			sc_msg_key;
	u16			sc_msg_type;

#ifdef CONFIG_DEBUG_FS
	struct list_head        sc_net_debug_item;
	ktime_t			sc_tv_timer;
	ktime_t			sc_tv_data_ready;
	ktime_t			sc_tv_advance_start;
	ktime_t			sc_tv_advance_stop;
	ktime_t			sc_tv_func_start;
	ktime_t			sc_tv_func_stop;
#endif
#ifdef CONFIG_PMDFC_FS_STATS
	ktime_t			sc_tv_acquiry_total;
	ktime_t			sc_tv_send_total;
	ktime_t			sc_tv_status_total;
	u32			sc_send_count;
	u32			sc_recv_count;
	ktime_t			sc_tv_process_total;
#endif
#endif
	struct mutex		sc_send_lock;
};
