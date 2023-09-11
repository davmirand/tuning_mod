void fMake_Binn_Object(struct PeerMsg *pMsg, binn * obj)
{
        binn_object_set_uint32(obj, "msg_no", pMsg->msg_no);
        binn_object_set_uint32(obj, "seq_no", pMsg->seq_no);
        binn_object_set_uint32(obj, "value", pMsg->value);
        binn_object_set_uint32(obj, "hop_latency", pMsg->hop_latency);
        binn_object_set_uint32(obj, "queue_occupancy", pMsg->queue_occupancy);
        binn_object_set_uint32(obj, "switch_id", pMsg->switch_id);
        binn_object_set_str(obj, "timestamp", pMsg->timestamp);
        binn_object_set_str(obj, "msg", pMsg->msg);
       
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
        strcpy(pMsg->timestamp,binn_object_str(obj, "timestamp"));
        strcpy(pMsg->msg, binn_object_str(obj, "msg"));
        //pMsg->pts = binn_object_str(obj, "timestamp");
        //pMsg->msg = binn_object_str(obj, "msg");
       
       	return;
}
