/*!
 * Setting of workqueue
 *
 * @param[in] void
 * @return 0=setting success, -1=setting fail
 */
int init_frizz_workqueue(void);

/*!
 * Delete workqueue
 *
 * @param[in] void
 * @return void
 */
void delete_frizz_workqueue(void);

/*!
 * frizz FW is recieved packet data. (delay process)
 *
 * @param[in] packet data
 * @return 0=success, -1=fail
 */
int create_frizz_workqueue(void*);

/*!
 * analysis frizz fifo data
 *
 * @param[in] void
 * @return 0=success, -1=fail
 */
int workqueue_analysis_fifo(void);

/*!
 * read frizz hw fifo data
 *
 * @param[in] void
 * @return 0=success, -1=fail
 */
int workqueue_read_fifo(void);
int workqueue_download_firmware(unsigned int chip_orientation);
int send_wakeup_cmd(void);
