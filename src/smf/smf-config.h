typedef struct smf_config_s {
    struct {
        ogs_sockaddr_t *addr;
        ogs_sockaddr_t *addr6;
        int port;
        int port6;
    } s8;

    struct {
        ogs_sockaddr_t *addr;
        ogs_sockaddr_t *addr6;
        int port;
        int port6;
    } s5c;
} 