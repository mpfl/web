#ifndef __WIFI_MGR_H__
#define __WIFI_MGR_H__
/* WIFI API BEGIN*/
/** The number of times that the WLAN Connection Manager will look for a
 *  network before giving up. */
#define WLAN_RESCAN_LIMIT				1
/** The minimum length for network names, see struct wlan_network.  This must
 *  be between 1 and \ref WLAN_NETWORK_NAME_MAX_LENGTH */
#define WLAN_NETWORK_NAME_MIN_LENGTH	1
/** The space reserved for storing network names, see struct wlan_network */
#define WLAN_NETWORK_NAME_MAX_LENGTH	32
/** The space reserved for storing PSK (password) phrases. */
#define WLAN_PSK_MAX_LENGTH				128
/** The size of the list of known networks maintained by the WLAN Connection
 *  Manager. */
#define WLAN_MAX_KNOWN_NETWORKS			10
/** Length of a pairwise master key (PMK).  It's always 256 bits (32 Bytes) */
#define WLAN_PMK_LENGTH				32

#define IEEEtypes_SSID_SIZE            32
#define IEEEtypes_ADDRESS_SIZE         6
/* Address types to be used by the element wlan_ip_config.addr_type below
 */
#define ADDR_TYPE_STATIC	0
#define ADDR_TYPE_DHCP		1
#define ADDR_TYPE_LLA		2

/** WLAN Connection Manager event reason */
enum wlan_event_reason {
	/** The WLAN Connection Manager has successfully connected to a network and
	 *  is now in the \ref WLAN_CONNECTED state. */
	WLAN_REASON_SUCCESS,
	/** The WLAN Connection Manager could not find the network that it was
	 *  connecting to (or it has tried all known networks and failed to connect
	 *  to any of them) and it is now in the \ref WLAN_DISCONNECTED state. */
	WLAN_REASON_NETWORK_NOT_FOUND,
	/** The WLAN Connection Manager failed to authenticate with the network
	 *  and is now in the \ref WLAN_DISCONNECTED state. */
	WLAN_REASON_NETWORK_AUTH_FAILED,
	/* This event will be received only on DHCP lease renewal*/
	WLAN_REASON_ADDRESS_SUCCESS,
	/** The WLAN Connection Manager failed to obtain an IP address
	 *  or TCP stack configuration has failed or the IP address
	 *  configuration was lost due to a DHCP error.  The system is
	 *  now in the \ref WLAN_DISCONNECTED state. */
	WLAN_REASON_ADDRESS_FAILED,
	/** The WLAN Connection Manager has lost the link to the current network. */
	WLAN_REASON_LINK_LOST,
	/** The WLAN Connection Manager has disconnected from the current network
	 *  (or has canceled a connection attempt) by request and is now in the
	 *  WLAN_DISCONNECTED state. */
	WLAN_REASON_USER_DISCONNECT,
	/** The WLAN Connection Manager has initialized and is ready for use.  That
	 *  is, it's now possible to scan or to connect to a network. */
	WLAN_REASON_INITIALIZED,
	/** The WLAN Connection Manager has failed to initialize and is therefore
	 *  not running.  It is not possible to scan or to connect to a network.  The
	 *  WLAN Connection Manager should be stopped and started again via
	 *  wlan_stop() and wlan_start() respectively. */
	WLAN_REASON_INITIALIZATION_FAILED,
	/** The WLAN Connection Mananger has entered power save mode. */
	WLAN_REASON_PS_ENTER,
	/** The WLAN Connection Mananger has exited from power save mode. */
	WLAN_REASON_PS_EXIT,
	/** The WLAN Connection Manager has started uAP */
	WLAN_REASON_UAP_SUCCESS,
	/** The WLAN Connection Manager has failed to start uAP */
	WLAN_REASON_UAP_START_FAILED,
	/** The WLAN Connection Manager has failed to stop uAP */
	WLAN_REASON_UAP_STOP_FAILED,
	/** The WLAN Connection Manager has stopped uAP */
	WLAN_REASON_UAP_STOPPED,
};
/** Network wireless mode */
enum wlan_mode {
	/** Infrastructure network.  The system will act as a station connected to
	 *  an Access Point. */
	WLAN_MODE_INFRASTRUCTURE = 0,
	/** Ad-hoc network.  The system will act as an ad-hoc node and connect to
	 *  an existing ad-hoc network if one is available, otherwise it may start
	 *  an ad-hoc network. */
	WLAN_MODE_ADHOC,
	/** uAP (micro-AP) network.  The system will act as an uAP node to
	 * which other Wireless clients can connect. */
	WLAN_MODE_UAP,

};
/** Network security type */
enum wlan_security_type {
	/** The network does not use security. */
	WLAN_SECURITY_NONE,
	/** The network uses WEP security with open key. */
	WLAN_SECURITY_WEP_OPEN,
	/** The network uses WEP security with shared key. */
	WLAN_SECURITY_WEP_SHARED,
	/** The network uses WPA security with PSK. */
	WLAN_SECURITY_WPA,
	/** The network uses WPA2 security with PSK. */
	WLAN_SECURITY_WPA2,
	/** The network uses AUTO security. Select security type as AP. (haibo add) */
	WLAN_SECURITY_AUTO,
	/** The network uses WPS. WPS support wild scan (haibo add) */
	WLAN_SECURITY_WPS,
	/** The network uses WPA or WPA2 security with PSK. */
	WLAN_SECURITY_WPA12,
};

/** Network IP configuration. 
 *
 *  This data structure represents an IP address, including the default
 *  gateway and subnet mask. */
struct wlan_ip_config {
	/** Set to \ref ADDR_TYPE_DHCP to use DHCP to obtain the IP address or
	 *  \ref ADDR_TYPE_STATIC to use a static IP. In case of static IP
	 *  Address ip, gw, netmask and dns members must be specified.  When
	 *  using DHCP, the ip, gw, netmask and dns are overwritten by the
	 *  values obtained from the DHCP Server. They should be zeroed out if
	 *  not used. */
	unsigned addr_type:2;
	/** The system's IP address in network order. */
	unsigned ip;
	/** The system's default gateway in network order. */
	unsigned gw;
	/** The system's subnet mask in network order. */
	unsigned netmask;
	/** The system's primary dns server in network order. */
	unsigned dns1;
	/** The system's secondary dns server in network order. */
	unsigned dns2;
};
/** Network security configuration */
struct wlan_network_security {
	/** Type of network security to use specified by enum
	 * wlan_security_type. */
	enum wlan_security_type type;
	/** Pre-shared key (network password).  For WEP networks this is a hex byte
	 * sequence of length psk_len, for WPA and WPA2 networks this is an ASCII
	 * pass-phrase of length psk_len.  This field is ignored for networks with no
	 * security. */
	char psk[WLAN_PSK_MAX_LENGTH];
	/** Length of the WEP key or WPA/WPA2 pass phrase, 0 to \ref
	 * WLAN_PSK_MAX_LENGTH.  Ignored for networks with no security. */
	char psk_len;
	/** Pairwise Master Key.  When pmk_valid is set, this is the PMK calculated
	 * from the PSK for WPA/PSK networks.  If pmk_valid is not set, this field
	 * is not valid.  When adding networks with \ref wlan_add_network, users
	 * can initialize pmk and set pmk_valid in lieu of setting the psk.  After
	 * successfully connecting to a WPA/PSK network, users can call \ref
	 * wlan_get_current_network to inspect pmk_valid and pmk.  Thus, the pmk
	 * value can be populated in subsequent calls to \ref wlan_add_network.
	 * This saves the CPU time required to otherwise calculate the PMK.
	 */
	char pmk[WLAN_PMK_LENGTH];

	/** Flag reporting whether pmk is valid or not. */
	char pmk_valid;
};

/** WLAN Network
 *
 *  This data structure represents a WLAN network. It consists of an arbitrary
 *  name, WiFi configuration, and IP address configuration.  */
struct wlan_network {
	/** The name of this network.  Each network that is added to the WLAN
	 *  Connection Manager must have a unique name of minimum length
	 *  WLAN_NETWORK_NAME_MIN_LENGTH. */
	char name[WLAN_NETWORK_NAME_MAX_LENGTH];
	/** The network SSID, represented as a C string of up to 32 characters
	 *  in length.  Set the first byte of the SSID to NULL (a 0-length
	 *  string) to use only the BSSID to find the network and obtain the
	 *  SSID, if any, from the Access Point. */
	char ssid[IEEEtypes_SSID_SIZE + 1];
	/** The network BSSID, represented as a 6-byte array.  Set all 6 bytes to 0
	 *  to use any BSSID, in which case only the SSID will be used to find the
	 *  network and the BSSID will be obtained from the Access Point. */
	char bssid[IEEEtypes_ADDRESS_SIZE];
	/** The channel on which to find the network.  Set this to 0 to allow the
	 *  network to be found on any channel. */
	unsigned int channel;
	/** The network wireless mode enum wlan_mode.  Set this to specify what
	 *  type of wireless network mode to use. */
	enum wlan_mode mode;
	/** The network security configuration specified by struct
	 * wlan_network_security. */
	struct wlan_network_security security;
	/** The network IP address configuration specified by struct
	 * wlan_ip_config. */
	struct wlan_ip_config address;

	/** Private Fields */

	/* If set to 1, the ssid field contains the spcific SSID for this network.
	 * The WLAN Connection Manager will only connect to networks whose SSID
	 * matches.  If set to 0, the ssid field contents are not used when
	 * deciding whether to connect to a network, the BSSID field is used
	 * instead and any network whose BSSID matches is accepted. 
	 *
	 * This field will be set to 1 if the network is added with the SSID
	 * specified (not an empty string), otherwise it is set to 0. */
	unsigned ssid_specific:1;

	/** If set to 1, the bssid field contains the specific BSSID for this 
	 *  network.  The WLAN Connection Manager will not connect to any other 
	 *  network with the same SSID unless the BSSID matches.  If set to 0, the 
	 *  WLAN Connection Manager will connect to any network whose SSID matches.
	 *
	 *  This field will be set to 1 if the network is added with the BSSID
	 *  specified (not set to all zeroes), otherwise it is set to 0. */
	unsigned bssid_specific:1;

	/* If set to 1, the channel field contains the specific channel for this
	 * network.  The WLAN Connection Manager will not look for this network on
	 * any other channel.  If set to 0, the WLAN Connection Manager will look
	 * for this network on any available channel. 
	 *
	 * This field will be set to 1 if the network is added with the channel
	 * specified (not set to 0), otherwise it is set to 0. */
	unsigned channel_specific:1;
};

/** enum : WPS session commands */
enum wps_command_session {
	/** Command to start WPS PIN session */
	WPS_CMD_PIN = 0,
	/** Command to start WPS PBC session */
	WPS_CMD_PBC
};

/** This structure is passed to \ref wps_send_command to start PIN or PBC
  * session. wps_pin is ignored if the command is \ref CMD_WPS_PBC.
  * A pre-obtained wlan_scan_result with WPS enabled network information
  * needs to be provided to begin session attempt. */
struct wps_start_command {
	/* Network name */
	char name[32];
	/** WPS command */
	enum wps_command_session command;
	char ssid[33];
	char bssid[6];
	/** 8 digit PIN for WPS PIN mode, ignored for PBC */
	unsigned int wps_pin;
};

/* WLAN Connection Manager API */
/** Add a network to the WLAN Connection Manager's list of known networks.
 *
 *  This function copies the contents of \a network to the list of
 *  known networks in the WLAN Connection Manager.  The network's
 *  'name' field must be unique and between \ref
 *  WLAN_NETWORK_NAME_MIN_LENGTH and \ref WLAN_NETWORK_NAME_MAX_LENGTH
 *  characters.  The network must specify at least an SSID or BSSID.
 *  The WLAN Connection Manager may store up to
 *  \ref WLAN_MAX_KNOWN_NETWORKS networks.
 *
 *  \note This function is synchronous.
 *
 *  \note This function may be called regardless of whether the WLAN Connection
 *  Manager service is running, however if the WLAN Connection Manager is
 *  running, wlan_add_network may only be called when the system is in the
 *  \ref WLAN_DISCONNECTED or \ref WLAN_CONNECTED state.
 *
 *  \param[in] network A pointer to the network that will be copied to the list
 *                 of known networks in the WLAN Connection Manager.
 *
 *  \return WLAN_ERROR_NONE if the contents pointed to by \a network have been 
 *          added to the WLAN Connection Manager.  
 * \return WLAN_ERROR_PARAM if \a network is NULL or the network name
 *          is not unique or the network name length is not valid.  
 * \return WLAN_ERROR_NOMEM if there was no room to add the network.  
 * \return WLAN_ERROR_STATE is returned if the WLAN Connection Manager
 *          was running and not in the \ref WLAN_DISCONNECTED or
 *          \ref WLAN_CONNECTED state. 
 */
int wlan_add_network(struct wlan_network *network);

/** Remove a network from the WLAN Connection Manager's list of known networks.
 *
 *  This function removes a network (identified by its name) from the WLAN
 *  Connection Manager, disconnecting from that network if needed.
 *
 *  \note This function is asynchronous if it is called while the WLAN
 *  Connection Manager is running and connected to the network to be removed.
 *  In that case, the WLAN Connection Manager will disconnect from the network
 *  and generate an event with reason \ref WLAN_REASON_USER_DISCONNECT.  This
 *  function is synchronous otherwise.
 *
 *  \note This function may be called regardless of whether the WLAN Connection
 *  Manager service is running, however if the WLAN Connection Manager is
 *  running, wlan_add_network may only be called when the system is in the
 *  \ref WLAN_DISCONNECTED or \ref WLAN_CONNECTED state.
 *
 *  \param[in] name A pointer to the C string representing the name of the
 *              network to remove.
 *
 *  \return WLAN_ERROR_NONE if the network named \a name was removed from the
 *          WLAN Connection Manager.  Otherwise, the network is not removed and
 *  \return WLAN_ERROR_STATE is returned if the WLAN Connection Manager was 
 *          running and not in the \ref WLAN_DISCONNECTED or \ref
 *          WLAN_CONNECTED states.
 * \return  WLAN_ERROR_PARAM if \a name is NULL or the network was not found in
 *          the list of known networks.  
 * \return  WLAN_ERROR_ACTION is returned if an internal error occured
 *          while trying to disconnect from the network specified for
 *          removal. 
 */
int wlan_remove_network(const char *name);

/** Connect to a network
 *
 *  This function causes the WLAN Connection Manager to try to connect to a
 *  network specified by \a name or, if \a name is not specified, to any
 *  network, starting with the first network in the list of known networks.
 *  This will generate an event when the connection process has completed or
 *  failed.  When connecting to a specific network, the event refers to the
 *  connection attempt to that network.  When connecting to any network, the
 *  WLAN Connection Manager generates an event once it has succeeded in
 *  connecting to one of the known networks or once it has tried and failed to
 *  connect to every known network.
 *
 *  Calling this function when the WLAN Connection Manager is in the
 *  \ref WLAN_DISCONNECTED state will, if successful, cause the WLAN Connection Manager
 *  to transition into the \ref WLAN_CONNECTING state.  If the connection attempt
 *  succeeds, the WLAN Connection Manager will transition to the WLAN_CONNECTED
 *  state, otherwise it will return to the \ref WLAN_DISCONNECTED state.  If this
 *  function is called while the WLAN Connection Manager is in the
 *  \ref WLAN_CONNECTING or \ref WLAN_CONNECTED state, the WLAN
 *  Connection Manager will first cancel its connection attempt or
 *  disconnect from the network, respectively, and generate an event
 *  with reason \ref WLAN_REASON_USER_DISCONNECT.  This will be
 *  followed by a second event that reports the result of the connection
 *  attempt.
 *
 *  \note This function is asynchronous provided that no error occurs and a
 *  connection attempt is made.  It is synchronous in the event of an error, in
 *  that case a connection will not be attempted and no callbacks will be
 *  generated.
 *
 *  \note This function may only be called while the WLAN Connection Manager
 *  service is running.
 *
 *  \param[in] name A pointer to a C string representing the name of the network
 *              to connect to or NULL to connect to any known network.
 *
 *  \return WLAN_ERROR_NONE if a connection is being attempted, otherwise a
 *          connection is not being attempted and the WLAN Connection Manager
 *          will not generate an event.  In that case, the 
 *          possible error codes are:   WLAN_ERROR_STATE if the WLAN 
 *          Connection Manager was not running.  WLAN_ERROR_PARAM if there are 
 *          no known networks to connect to or the network specified by \a name 
 *          is not in the list of known networks.  WLAN_ERROR_ACTION if an 
 *          internal error has occurred.
 */
int wlan_connect(char *name);

/** Start a Wireless network
 *
 *  This function causes the WLAN Connection Manager to try to start a network
 *  specified by \a name.  The specified network must have a valid SSID.
 *  Calling this function is the equivalent of calling wlan_connect() on such a
 *  network, however the WLAN Connection Manager will not attempt to try to find
 *  the specified network and join it before trying to start the network.
 *  wlan_start_network should therefore be used when it is preferable to start a
 *  network immediately.  This function may cause a disconnect from the current
 *  network or cancel an existing connection attempt, see wlan_connect() for
 *  details.
 *
 *  \note This function is asynchronous provided that no error occurs and a
 *  connection attempt is made.  It is synchronous in the event of an error, in
 *  that case a connection will not be attempted and no callbacks will be
 *  generated.
 *
 *  \note This function may only be called while the WLAN Connection Manager
 *  service is running.
 *
 *  \param[in] name A pointer to a C string representing the name of the network
 *              to connect to.
 *
 *  \return WLAN_ERROR_NONE if a connection is being attempted (the WLAN
 *          Connection Manager is attempting to start the specified network),
 *          otherwise no connection is being attempted and the WLAN Connection
 *          Manager will not generate an event.  In that case, WLAN_ERROR_PARAM
 *          is retured to indicate that \a name was NULL  or the network \a
 *          name was not found or it not have a specified SSID.
 */
int wlan_start_network(const char *name);

/** Stop a Wireless network
 *
 *  This function causes the WLAN Connection Manager to try to stop a network
 *  specified by \a name.  The specified network must be an uAP network and
 *  it must have a specified SSID.
 *
 *  \note This function may only be called while the WLAN Connection Manager
 *  service is running.
 *
 *  \param[in] name A pointer to a C string representing the name of the network
 *              to stop.
 *
 *  \return WLAN_ERROR_NONE if there network is stopped. WLAN_ERROR_PARAM
 *          is retured to indicate that \a name was NULL  or the network \a
 *          name was not found or that the network \a name is not an uAP
 *          network or it is an uAP network but does not have a specified
 *          SSID.
 */
int wlan_stop_network(const char *name);

/** Disconnect from the current network.
 *
 *  This function causes the WLAN Connection Manager to disconnect from its
 *  currently-connected network (or cancel an in-progress connection attempt)
 *  and return to the \ref WLAN_DISCONNECTED state.  Calling this function has no effect
 *  if the WLAN Connection Manager was already disconnected.
 *
 *  \note This function is asynchronous if called when the WLAN Connection
 *  Manager is in the \ref WLAN_CONNECTING or \ref WLAN_CONNECTED
 *  state.  In that case, the WLAN  Connection Manager will cancel its
 *  connection attempt or disconnect from the network and generate an
 *  event with reason code \ref WLAN_REASON_USER_DISCONNECT.
 *  Otherwise, this function is synchronous and has no effect.
 *
 *  \note This function may only be called when the WLAN Connection Manager is
 *  running.
 *
 * \return  WLAN_ERROR_STATE if the WLAN Connection Manager was not running.
 * \return  WLAN_ERROR_NONE if the WLAN Connection Manager was running.
 */
int wlan_disconnect(void);

int wlan_wps_start(struct wps_start_command *cmd);

void wlan_get_tx_power(int *min, int *max, int *cur);
int wlan_set_tx_power(int level);

int wlan_get_mac_address(unsigned char *mac);

void user_scan(void);


/* User callback function */
void mxchipWifiStatusHandler(int event);
void mxchipPMKHandler(struct wlan_network *net);

/* WIFI API END*/
#endif // __WIFI_MGR_H__ END
