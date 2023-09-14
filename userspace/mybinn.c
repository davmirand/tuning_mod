void fMake_Binn_Object(struct PeerMsg *pMsg, binn * obj)
{
#if 0
	unsigned int msg_no;
        unsigned int seq_no;
        unsigned int value;
        unsigned int hop_latency;
        unsigned int queue_occupancy;
        unsigned int switch_id;
        char timestamp[MS_CTIME_BUF_LEN];
        char msg[80];
        char * pts;
        char * ptimes;
        char * pm;
#endif	
	FILE * thisptr = 0;
	if (IamClient)
		thisptr = pHpnClientLogPtr;
	else
		thisptr = tunLogPtr;
	
        binn_object_set_uint32(obj, "msg_no", pMsg->msg_no);
        binn_object_set_uint32(obj, "seq_no", pMsg->seq_no);
        binn_object_set_uint32(obj, "value", pMsg->value);
        binn_object_set_uint32(obj, "hop_latency", pMsg->hop_latency);
        binn_object_set_uint32(obj, "queue_occupancy", pMsg->queue_occupancy);
        binn_object_set_uint32(obj, "switch_id", pMsg->switch_id);
        binn_object_set_str(obj, "timestamp", pMsg->timestamp);
        binn_object_set_str(obj, "msg", pMsg->msg);

#if 0
	fprintf(thisptr,"msg no = %u, seq_no = %u, value = %u, hop_latency= %u, msg = %s***\n", pMsg->msg_no, pMsg->seq_no, pMsg->value, pMsg->hop_latency, pMsg->msg);
	fflush (thisptr);
	
	msg_no = binn_object_uint32 (obj,"msg_no");
        seq_no = binn_object_uint32 (obj,"seq_no");
        value = binn_object_uint32 (obj,"value");
        hop_latency = binn_object_uint32 (obj,"hop_latency");
        queue_occupancy = binn_object_uint32 (obj,"queue_occupancy");
        switch_id = binn_object_uint32 (obj,"switch_id");
        pm =  binn_object_str(obj, "msg");
	
	fprintf(thisptr,"***NEXT msg no = %u, seq_no = %u, value = %u, hop_latency= %u, msg = %s***\n", msg_no, seq_no, value, hop_latency, pm);
	fflush (thisptr);
#endif
       	return;
}

void fRead_Binn_Object(struct PeerMsg *pMsg, binn * obj)
{

        pMsg->msg_no = binn_object_uint32(obj, "msg_no");
        pMsg->seq_no = binn_object_uint32(obj, "seq_no");
        pMsg->value = binn_object_uint32(obj, "value");
        pMsg->hop_latency = binn_object_uint32(obj, "hop_latency");
        pMsg->queue_occupancy = binn_object_uint32(obj, "queue_occupancy");
        pMsg->switch_id = binn_object_uint32(obj, "switch_id");
        pMsg->ptimes = binn_object_str(obj, "timestamp");
        //strcpy(pMsg->timestamp, pMsg->ptimes);
        pMsg->pm = binn_object_str(obj, "msg");
        //strcpy(pMsg->msg, pMsg->pm);
       
       	return;
}
