#include <unistd.h>

#include <glib.h>
#include "../../librdkafka/src/rdkafka.h"
void fUseKafka(void)
{

	char hostname[128];
	char errstr[512];

	rd_kafka_conf_t *conf = rd_kafka_conf_new();

	if (gethostname(hostname, sizeof(hostname))) 
	{
 		fprintf(stderr, "%% Failed to lookup hostname\n");
 		exit(1);
	}

	if (rd_kafka_conf_set(conf, "client.id", hostname, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) 
	{
 		fprintf(stderr, "%% %s\n", errstr);
 		exit(1);
	}

	if (rd_kafka_conf_set(conf, "group.id", "foo", errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) 
	{
 		fprintf(stderr, "%% %s\n", errstr);
 		exit(1);
	}

	if (rd_kafka_conf_set(conf, "bootstrap.servers", "host1:9092,host2:9092", errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) 
	{
 		fprintf(stderr, "%% %s\n", errstr);
 		exit(1);
	}

	/* Create Kafka consumer handle */
	rd_kafka_t *rk;
	if (!(rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr)))) 
	{
 		fprintf(stderr, "%% Failed to create new consumer: %s\n", errstr);
 		exit(1);
	}

}


int main()
{
	fUseKafka();	
	return 0;
}	
