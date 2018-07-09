# private_dainty_mt
&lt;dainty::mt> is a multithreading library that is used by the dainty framework

1. <dainty::mt::thread>
1.1   t_thread
1.1.1 t_logic
1.2   t_detachedthread
1.2.1 t_logic

2. <dainty::mt::command>
2.1 t_client
2.2 t_fd_processor
2.3 t_condvar_processor

many multithreading related services must still be ported and added to dainty::mt.

1. <dainty::mt::linked_queue>
1.1 t_client
1.2 t_fd_processor
1.3 t_condvar_processor

2. <dainty::mt::event>
2.1 t_client
2.2 t_fd_processor
2.3 t_condvar_processor

3. <dainty::mt::notify_change>
3.1 t_client
3.2 t_fd_processor
3.3 t_condvar_processor

4. <dainty::mt::mqueue>
4.1 t_mq_client
4.2 t_mq_processor

etc...
