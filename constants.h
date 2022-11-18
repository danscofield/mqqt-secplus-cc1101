// This is the SPI pin configuration
const int pCSN = 18;
const int pMOSI = 35;
const int pSCK = 36;
const int pMISO = 37;
const int pGDO0 = 17;


#define HEALTHCHECK_DEVICE_NAME "gargaedooropener"
#define AWS_IOT_SUBSCRIBE_TOPIC "gdo/requests"
#define AWS_IOT_PUBLISH_TOPIC "gdo/responses"
#define AWS_IOT_HEALTHCHECK_TOPIC "gdo/health"

const char AWS_IOT_ENDPOINT[] = "<fillmein>.iot.us-east-1.amazonaws.com";
const int AWS_IOT_PORT = 8883;
