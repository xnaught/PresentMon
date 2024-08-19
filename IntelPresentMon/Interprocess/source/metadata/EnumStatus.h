#pragma once
#include "../../../PresentMonAPI2/PresentMonAPI.h"

// enum annotation (enum_name_fragment, key_name_fragment, name, short_name, description)
#define ENUM_KEY_LIST_STATUS(X_) \
		X_(STATUS, SUCCESS, "Success", "", "Operation succeeded") \
		X_(STATUS, FAILURE, "Failure", "", "Operation failed") \
		X_(STATUS, SESSION_NOT_OPEN, "Session Not Open", "", "Operation requires an open session") \
		X_(STATUS, SERVICE_ERROR, "Service Error", "", "An error occurred within the service") \
		X_(STATUS, INVALID_ETL_FILE, "Invalid ETL File", "", "") \
		X_(STATUS, DATA_LOSS, "Data Loss", "", "Data was overwritten before being read by client") \
		X_(STATUS, NO_DATA, "No Data", "", "No data is available") \
		X_(STATUS, INVALID_PID, "Invalid PID", "", "PID does not exist or does not match a process being tracked") \
		X_(STATUS, STREAM_ALREADY_EXISTS, "Stream Already Exists", "", "Tried to open stream for a process already being tracked") \
		X_(STATUS, UNABLE_TO_CREATE_NSM, "Unable to Create NSM", "", "Service failed to create shrared memory for data transfer") \
		X_(STATUS, INVALID_ADAPTER_ID, "Invalid Adapter ID", "", "Graphics adapter ID provided does not match any ID in service") \
		X_(STATUS, OUT_OF_RANGE, "Out of Range", "", "Value falls outside of the acceptable range") \
		X_(STATUS, INSUFFICIENT_BUFFER, "Insufficient Buffer", "", "Buffer is not large enough to hold all output data") \
		X_(STATUS, PIPE_ERROR, "Pipe Error", "", "An error occurred in connecting to or communicating over named pipes")