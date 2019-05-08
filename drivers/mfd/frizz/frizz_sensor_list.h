
/*!
 * convert Android Sensor type to MCC Sensor type
 *
 * @param[in] sensor code (Android Sensor type)
 * @return sensor id (MCC Sensor type)
 */
int convert_code_to_id(int);

/*!
 * convert MCC Sensor type to Android Sensor type
 *
 * @param[in] sensor id (MCC Sensor type)
 * @return sensor code  (Android Sensor type)
 */
int convert_id_to_code(int);

