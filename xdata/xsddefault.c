/*****************************************************************************
 *
 * XSDDEFAULT.C - Default external status data input routines for Nagios
 *
 * Copyright (c) 2000-2006 Ethan Galstad (nagios@nagios.org)
 * Last Modified:   02-28-2006
 *
 * License:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *****************************************************************************/


/*********** COMMON HEADER FILES ***********/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/locations.h"
#include "../include/statusdata.h"

#ifdef NSCORE
#include "../include/nagios.h"
#endif

#ifdef NSCGI
#include "../include/cgiutils.h"
#endif


/**** IMPLEMENTATION SPECIFIC HEADER FILES ****/
#include "xsddefault.h"



#ifdef NSCGI
time_t program_start;
int daemon_mode;
time_t last_command_check;
time_t last_log_rotation;
int enable_notifications;
int execute_service_checks;
int accept_passive_service_checks;
int execute_host_checks;
int accept_passive_host_checks;
int enable_event_handlers;
int obsess_over_services;
int obsess_over_hosts;
int check_service_freshness;
int check_host_freshness;
int enable_flap_detection;
int enable_failure_prediction;
int process_performance_data;
int nagios_pid;
#endif

#ifdef NSCORE
extern time_t program_start;
extern int nagios_pid;
extern int daemon_mode;
extern time_t last_command_check;
extern time_t last_log_rotation;
extern int enable_notifications;
extern int execute_service_checks;
extern int accept_passive_service_checks;
extern int execute_host_checks;
extern int accept_passive_host_checks;
extern int enable_event_handlers;
extern int obsess_over_services;
extern int obsess_over_hosts;
extern int check_service_freshness;
extern int check_host_freshness;
extern int enable_flap_detection;
extern int enable_failure_prediction;
extern int process_performance_data;
extern int aggregate_status_updates;

extern char *macro_x[MACRO_X_COUNT];

extern host *host_list;
extern service *service_list;
extern contact *contact_list;

extern unsigned long  next_comment_id;
extern unsigned long  next_downtime_id;
extern unsigned long  next_event_id;
extern unsigned long  next_notification_id;

extern unsigned long  modified_host_process_attributes;
extern unsigned long  modified_service_process_attributes;
extern char           *global_host_event_handler;
extern char           *global_service_event_handler;
#endif


char *xsddefault_status_log=NULL;
char *xsddefault_temp_file=NULL;



/******************************************************************/
/***************** COMMON CONFIG INITIALIZATION  ******************/
/******************************************************************/

/* grab configuration information */
int xsddefault_grab_config_info(char *config_file){
	char *input=NULL;
	mmapfile *thefile;
#ifdef NSCGI
	char *input2=NULL;
	mmapfile *thefile2;
	char *temp_buffer;
#endif


	/*** CORE PASSES IN MAIN CONFIG FILE, CGIS PASS IN CGI CONFIG FILE! ***/

	/* open the config file for reading */
	if((thefile=mmap_fopen(config_file))==NULL)
		return ERROR;

	/* read in all lines from the main config file */
	while(1){

		/* free memory */
		my_free((void **)&input);

		/* read the next line */
		if((input=mmap_fgets(thefile))==NULL)
			break;

		strip(input);

		/* skip blank lines and comments */
		if(input[0]=='#' || input[0]=='\x0')
			continue;

#ifdef NSCGI
		/* CGI needs to find and read contents of main config file, since it was passed the name of the CGI config file */
		if(strstr(input,"main_config_file")==input){

			temp_buffer=strtok(input,"=");
			temp_buffer=strtok(NULL,"\n");
			if(temp_buffer==NULL)
				continue;

			if((thefile2=mmap_fopen(temp_buffer))==NULL)
				continue;

			/* read in all lines from the main config file */
			while(1){

				/* free memory */
				my_free((void **)&input2);

				/* read the next line */
				if((input2=mmap_fgets(thefile2))==NULL)
					break;

				strip(input2);

				/* skip blank lines and comments */
				if(input2[0]=='#' || input2[0]=='\x0')
					continue;

				xsddefault_grab_config_directives(input2);
			        }

			/* free memory and close the file */
			my_free((void **)&input2);
			mmap_fclose(thefile2);
		        }
#endif

#ifdef NSCORE
		/* core reads variables directly from the main config file */
		xsddefault_grab_config_directives(input);
#endif
	        }

	/* free memory and close the file */
	my_free((void **)&input);
	mmap_fclose(thefile);

	/* initialize locations if necessary */
	if(xsddefault_status_log==NULL)
		xsddefault_status_log=(char *)strdup(DEFAULT_STATUS_FILE);
	if(xsddefault_temp_file==NULL)
		xsddefault_temp_file=(char *)strdup(DEFAULT_TEMP_FILE);

	/* make sure we have what we need */
	if(xsddefault_status_log==NULL)
		return ERROR;
	if(xsddefault_temp_file==NULL)
		return ERROR;

#ifdef NSCORE
	/* save the status file macro */
	my_free((void **)&macro_x[MACRO_STATUSDATAFILE]);
	if((macro_x[MACRO_STATUSDATAFILE]=(char *)strdup(xsddefault_status_log)))
		strip(macro_x[MACRO_STATUSDATAFILE]);
#endif

	return OK;
        }


/* processes a single directive */
int xsddefault_grab_config_directives(char *input){
	char *temp_ptr=NULL;
	char *varname=NULL;
	char *varvalue=NULL;

	/* get the variable name */
	if((temp_ptr=my_strtok(input,"="))==NULL)
		return ERROR;
	if((varname=(char *)strdup(temp_ptr))==NULL)
		return ERROR;

	/* get the variable value */
	if((temp_ptr=my_strtok(NULL,"\n"))==NULL){
		my_free((void **)&varname);
		return ERROR;
	        }
	if((varvalue=(char *)strdup(temp_ptr))==NULL){
		my_free((void **)&varname);
		return ERROR;
	        }

	/* status log definition */
	if(!strcmp(varname,"status_file") || !strcmp(varname,"xsddefault_status_log"))
		xsddefault_status_log=(char *)strdup(temp_ptr);

	/* temp file definition */
	if(!strcmp(varname,"temp_file"))
		xsddefault_temp_file=(char *)strdup(temp_ptr);

	/* free memory */
	my_free((void **)&varname);
	my_free((void **)&varvalue);

	return OK;
        }



#ifdef NSCORE

/******************************************************************/
/********************* INIT/CLEANUP FUNCTIONS *********************/
/******************************************************************/


/* initialize status data */
int xsddefault_initialize_status_data(char *config_file){
	int result;

	/* grab configuration data */
	result=xsddefault_grab_config_info(config_file);
	if(result==ERROR)
		return ERROR;

	/* delete the old status log (it might not exist) */
	if(xsddefault_status_log)
		unlink(xsddefault_status_log);

	return OK;
        }


/* cleanup status data before terminating */
int xsddefault_cleanup_status_data(char *config_file, int delete_status_data){

	/* delete the status log */
	if(delete_status_data==TRUE && xsddefault_status_log){
		if(unlink(xsddefault_status_log))
			return ERROR;
	        }

	/* free memory */
	my_free((void **)&xsddefault_status_log);
	my_free((void **)&xsddefault_temp_file);

	return OK;
        }


/******************************************************************/
/****************** STATUS DATA OUTPUT FUNCTIONS ******************/
/******************************************************************/

/* write all status data to file */
int xsddefault_save_status_data(void){
	char *temp_file=NULL;
	customvariablesmember *temp_customvariablesmember=NULL;
	char *temp_buffer=NULL;
	host *temp_host=NULL;
	service *temp_service=NULL;
	contact *temp_contact=NULL;
	time_t current_time;
	int fd=0;
	FILE *fp=NULL;

	/* open a safe temp file for output */
	if(xsddefault_temp_file==NULL)
		return ERROR;
	asprintf(&temp_file,"%sXXXXXX",xsddefault_temp_file);
	if(temp_file==NULL)
		return ERROR;

	if((fd=mkstemp(temp_file))==-1){

		/* log an error */
		asprintf(&temp_buffer,"Error: Unable to create temp file for writing status data!\n");
		write_to_logs_and_console(temp_buffer,NSLOG_RUNTIME_ERROR,TRUE);
		my_free((void **)&temp_buffer);

		/* free memory */
		my_free((void **)&temp_file);

		return ERROR;
	        }
	fp=(FILE *)fdopen(fd,"w");
	if(fp==NULL){

		close(fd);
		unlink(temp_file);

		/* log an error */
		asprintf(&temp_buffer,"Error: Unable to open temp file '%s' for writing status data!\n",temp_file);
		write_to_logs_and_console(temp_buffer,NSLOG_RUNTIME_ERROR,TRUE);
		my_free((void **)&temp_buffer);

		/* free memory */
		my_free((void **)&temp_file);

		return ERROR;
	        }

	/* write version info to status file */
	fprintf(fp,"########################################\n");
	fprintf(fp,"#          NAGIOS STATUS FILE\n");
	fprintf(fp,"#\n");
	fprintf(fp,"# THIS FILE IS AUTOMATICALLY GENERATED\n");
	fprintf(fp,"# BY NAGIOS.  DO NOT MODIFY THIS FILE!\n");
	fprintf(fp,"########################################\n\n");

	time(&current_time);

	/* write file info */
	fprintf(fp,"info {\n");
	fprintf(fp,"\tcreated=%lu\n",current_time);
	fprintf(fp,"\tversion=%s\n",PROGRAM_VERSION);
	fprintf(fp,"\t}\n\n");

	/* save program status data */
	fprintf(fp,"program {\n");
	fprintf(fp,"\tmodified_host_attributes=%lu\n",modified_host_process_attributes);
	fprintf(fp,"\tmodified_service_attributes=%lu\n",modified_service_process_attributes);
	fprintf(fp,"\tnagios_pid=%d\n",nagios_pid);
	fprintf(fp,"\tdaemon_mode=%d\n",daemon_mode);
	fprintf(fp,"\tprogram_start=%lu\n",program_start);
	fprintf(fp,"\tlast_command_check=%lu\n",last_command_check);
	fprintf(fp,"\tlast_log_rotation=%lu\n",last_log_rotation);
	fprintf(fp,"\tenable_notifications=%d\n",enable_notifications);
	fprintf(fp,"\tactive_service_checks_enabled=%d\n",execute_service_checks);
	fprintf(fp,"\tpassive_service_checks_enabled=%d\n",accept_passive_service_checks);
	fprintf(fp,"\tactive_host_checks_enabled=%d\n",execute_host_checks);
	fprintf(fp,"\tpassive_host_checks_enabled=%d\n",accept_passive_host_checks);
	fprintf(fp,"\tenable_event_handlers=%d\n",enable_event_handlers);
	fprintf(fp,"\tobsess_over_services=%d\n",obsess_over_services);
	fprintf(fp,"\tobsess_over_hosts=%d\n",obsess_over_hosts);
	fprintf(fp,"\tcheck_service_freshness=%d\n",check_service_freshness);
	fprintf(fp,"\tcheck_host_freshness=%d\n",check_host_freshness);
	fprintf(fp,"\tenable_flap_detection=%d\n",enable_flap_detection);
	fprintf(fp,"\tenable_failure_prediction=%d\n",enable_failure_prediction);
	fprintf(fp,"\tprocess_performance_data=%d\n",process_performance_data);
	fprintf(fp,"\tglobal_host_event_handler=%s\n",(global_host_event_handler==NULL)?"":global_host_event_handler);
	fprintf(fp,"\tglobal_service_event_handler=%s\n",(global_service_event_handler==NULL)?"":global_service_event_handler);
	fprintf(fp,"\tnext_comment_id=%lu\n",next_comment_id);
	fprintf(fp,"\tnext_downtime_id=%lu\n",next_downtime_id);
	fprintf(fp,"\tnext_event_id=%lu\n",next_event_id);
	fprintf(fp,"\tnext_notification_id=%lu\n",next_notification_id);
	fprintf(fp,"\t}\n\n");


	/* save host status data */
	for(temp_host=host_list;temp_host!=NULL;temp_host=temp_host->next){

		fprintf(fp,"host {\n");
		fprintf(fp,"\thost_name=%s\n",temp_host->name);
		fprintf(fp,"\tmodified_attributes=%lu\n",temp_host->modified_attributes);
		fprintf(fp,"\tcheck_command=%s\n",(temp_host->host_check_command==NULL)?"":temp_host->host_check_command);
		fprintf(fp,"\tcheck_period=%s\n",(temp_host->check_period==NULL)?"":temp_host->check_period);
		fprintf(fp,"\tcheck_interval=%d\n",temp_host->check_interval);
		fprintf(fp,"\tevent_handler=%s\n",(temp_host->event_handler==NULL)?"":temp_host->event_handler);
		fprintf(fp,"\thas_been_checked=%d\n",temp_host->has_been_checked);
		fprintf(fp,"\tshould_be_scheduled=%d\n",temp_host->should_be_scheduled);
		fprintf(fp,"\tcheck_execution_time=%.3f\n",temp_host->execution_time);
		fprintf(fp,"\tcheck_latency=%.3f\n",temp_host->latency);
		fprintf(fp,"\tcheck_type=%d\n",temp_host->check_type);
		fprintf(fp,"\tcurrent_state=%d\n",temp_host->current_state);
		fprintf(fp,"\tlast_hard_state=%d\n",temp_host->last_hard_state);
		fprintf(fp,"\tplugin_output=%s\n",(temp_host->plugin_output==NULL)?"":temp_host->plugin_output);
		fprintf(fp,"\tlong_plugin_output=%s\n",(temp_host->long_plugin_output==NULL)?"":temp_host->long_plugin_output);
		fprintf(fp,"\tperformance_data=%s\n",(temp_host->perf_data==NULL)?"":temp_host->perf_data);
		fprintf(fp,"\tlast_check=%lu\n",temp_host->last_check);
		fprintf(fp,"\tnext_check=%lu\n",temp_host->next_check);
		fprintf(fp,"\tcurrent_attempt=%d\n",temp_host->current_attempt);
		fprintf(fp,"\tmax_attempts=%d\n",temp_host->max_attempts);
		fprintf(fp,"\tcurrent_event_id=%lu\n",temp_host->current_event_id);
		fprintf(fp,"\tlast_event_id=%lu\n",temp_host->last_event_id);
		fprintf(fp,"\tstate_type=%d\n",temp_host->state_type);
		fprintf(fp,"\tlast_state_change=%lu\n",temp_host->last_state_change);
		fprintf(fp,"\tlast_hard_state_change=%lu\n",temp_host->last_hard_state_change);
		fprintf(fp,"\tlast_time_up=%lu\n",temp_host->last_time_up);
		fprintf(fp,"\tlast_time_down=%lu\n",temp_host->last_time_down);
		fprintf(fp,"\tlast_time_unreachable=%lu\n",temp_host->last_time_unreachable);
		fprintf(fp,"\tlast_notification=%lu\n",temp_host->last_host_notification);
		fprintf(fp,"\tnext_notification=%lu\n",temp_host->next_host_notification);
		fprintf(fp,"\tno_more_notifications=%d\n",temp_host->no_more_notifications);
		fprintf(fp,"\tcurrent_notification_number=%d\n",temp_host->current_notification_number);
		fprintf(fp,"\tcurrent_notification_id=%lu\n",temp_host->current_notification_id);
		fprintf(fp,"\tnotifications_enabled=%d\n",temp_host->notifications_enabled);
		fprintf(fp,"\tproblem_has_been_acknowledged=%d\n",temp_host->problem_has_been_acknowledged);
		fprintf(fp,"\tacknowledgement_type=%d\n",temp_host->acknowledgement_type);
		fprintf(fp,"\tactive_checks_enabled=%d\n",temp_host->checks_enabled);
		fprintf(fp,"\tpassive_checks_enabled=%d\n",temp_host->accept_passive_host_checks);
		fprintf(fp,"\tevent_handler_enabled=%d\n",temp_host->event_handler_enabled);
		fprintf(fp,"\tflap_detection_enabled=%d\n",temp_host->flap_detection_enabled);
		fprintf(fp,"\tfailure_prediction_enabled=%d\n",temp_host->failure_prediction_enabled);
		fprintf(fp,"\tprocess_performance_data=%d\n",temp_host->process_performance_data);
		fprintf(fp,"\tobsess_over_host=%d\n",temp_host->obsess_over_host);
		fprintf(fp,"\tlast_update=%lu\n",current_time);
		fprintf(fp,"\tis_flapping=%d\n",temp_host->is_flapping);
		fprintf(fp,"\tpercent_state_change=%.2f\n",temp_host->percent_state_change);
		fprintf(fp,"\tscheduled_downtime_depth=%d\n",temp_host->scheduled_downtime_depth);
		/*
		fprintf(fp,"\tstate_history=");
		for(x=0;x<MAX_STATE_HISTORY_ENTRIES;x++)
			fprintf(fp,"%s%d",(x>0)?",":"",temp_host->state_history[(x+temp_host->state_history_index)%MAX_STATE_HISTORY_ENTRIES]);
		fprintf(fp,"\n");
		*/
		/* custom variables */
		for(temp_customvariablesmember=temp_host->custom_variables;temp_customvariablesmember!=NULL;temp_customvariablesmember=temp_customvariablesmember->next){
			if(temp_customvariablesmember->variable_name)
				fprintf(fp,"\t_%s=%d;%s\n",temp_customvariablesmember->variable_name,temp_customvariablesmember->has_been_modified,(temp_customvariablesmember->variable_value==NULL)?"":temp_customvariablesmember->variable_value);
		        }
		fprintf(fp,"\t}\n\n");
	        }

	/* save service status data */
	for(temp_service=service_list;temp_service!=NULL;temp_service=temp_service->next){

		fprintf(fp,"service {\n");
		fprintf(fp,"\thost_name=%s\n",temp_service->host_name);
		fprintf(fp,"\tservice_description=%s\n",temp_service->description);
		fprintf(fp,"\tmodified_attributes=%lu\n",temp_service->modified_attributes);
		fprintf(fp,"\tcheck_command=%s\n",(temp_service->service_check_command==NULL)?"":temp_service->service_check_command);
		fprintf(fp,"\tcheck_period=%s\n",(temp_service->check_period==NULL)?"":temp_service->check_period);
		fprintf(fp,"\tcheck_interval=%d\n",temp_service->check_interval);
		fprintf(fp,"\tretry_interval=%d\n",temp_service->retry_interval);
		fprintf(fp,"\tevent_handler=%s\n",(temp_service->event_handler==NULL)?"":temp_service->event_handler);
		fprintf(fp,"\thas_been_checked=%d\n",temp_service->has_been_checked);
		fprintf(fp,"\tshould_be_scheduled=%d\n",temp_service->should_be_scheduled);
		fprintf(fp,"\tcheck_execution_time=%.3f\n",temp_service->execution_time);
		fprintf(fp,"\tcheck_latency=%.3f\n",temp_service->latency);
		fprintf(fp,"\tcheck_type=%d\n",temp_service->check_type);
		fprintf(fp,"\tcurrent_state=%d\n",temp_service->current_state);
		fprintf(fp,"\tlast_hard_state=%d\n",temp_service->last_hard_state);
		fprintf(fp,"\tcurrent_attempt=%d\n",temp_service->current_attempt);
		fprintf(fp,"\tmax_attempts=%d\n",temp_service->max_attempts);
		fprintf(fp,"\tcurrent_event_id=%lu\n",temp_service->current_event_id);
		fprintf(fp,"\tlast_event_id=%lu\n",temp_service->last_event_id);
		fprintf(fp,"\tstate_type=%d\n",temp_service->state_type);
		fprintf(fp,"\tlast_state_change=%lu\n",temp_service->last_state_change);
		fprintf(fp,"\tlast_hard_state_change=%lu\n",temp_service->last_hard_state_change);
		fprintf(fp,"\tlast_time_ok=%lu\n",temp_service->last_time_ok);
		fprintf(fp,"\tlast_time_warning=%lu\n",temp_service->last_time_warning);
		fprintf(fp,"\tlast_time_unknown=%lu\n",temp_service->last_time_unknown);
		fprintf(fp,"\tlast_time_critical=%lu\n",temp_service->last_time_critical);
		fprintf(fp,"\tplugin_output=%s\n",(temp_service->plugin_output==NULL)?"":temp_service->plugin_output);
		fprintf(fp,"\tlong_plugin_output=%s\n",(temp_service->long_plugin_output==NULL)?"":temp_service->long_plugin_output);
		fprintf(fp,"\tperformance_data=%s\n",(temp_service->perf_data==NULL)?"":temp_service->perf_data);
		fprintf(fp,"\tlast_check=%lu\n",temp_service->last_check);
		fprintf(fp,"\tnext_check=%lu\n",temp_service->next_check);
		fprintf(fp,"\tcurrent_notification_number=%d\n",temp_service->current_notification_number);
		fprintf(fp,"\tcurrent_notification_id=%lu\n",temp_service->current_notification_id);
		fprintf(fp,"\tlast_notification=%lu\n",temp_service->last_notification);
		fprintf(fp,"\tnext_notification=%lu\n",temp_service->next_notification);
		fprintf(fp,"\tno_more_notifications=%d\n",temp_service->no_more_notifications);
		fprintf(fp,"\tnotifications_enabled=%d\n",temp_service->notifications_enabled);
		fprintf(fp,"\tactive_checks_enabled=%d\n",temp_service->checks_enabled);
		fprintf(fp,"\tpassive_checks_enabled=%d\n",temp_service->accept_passive_service_checks);
		fprintf(fp,"\tevent_handler_enabled=%d\n",temp_service->event_handler_enabled);
		fprintf(fp,"\tproblem_has_been_acknowledged=%d\n",temp_service->problem_has_been_acknowledged);
		fprintf(fp,"\tacknowledgement_type=%d\n",temp_service->acknowledgement_type);
		fprintf(fp,"\tflap_detection_enabled=%d\n",temp_service->flap_detection_enabled);
		fprintf(fp,"\tfailure_prediction_enabled=%d\n",temp_service->failure_prediction_enabled);
		fprintf(fp,"\tprocess_performance_data=%d\n",temp_service->process_performance_data);
		fprintf(fp,"\tobsess_over_service=%d\n",temp_service->obsess_over_service);
		fprintf(fp,"\tlast_update=%lu\n",current_time);
		fprintf(fp,"\tis_flapping=%d\n",temp_service->is_flapping);
		fprintf(fp,"\tpercent_state_change=%.2f\n",temp_service->percent_state_change);
		fprintf(fp,"\tscheduled_downtime_depth=%d\n",temp_service->scheduled_downtime_depth);
		/*
		fprintf(fp,"\tstate_history=");
		for(x=0;x<MAX_STATE_HISTORY_ENTRIES;x++)
			fprintf(fp,"%s%d",(x>0)?",":"",temp_service->state_history[(x+temp_service->state_history_index)%MAX_STATE_HISTORY_ENTRIES]);
		fprintf(fp,"\n");
		*/
		/* custom variables */
		for(temp_customvariablesmember=temp_service->custom_variables;temp_customvariablesmember!=NULL;temp_customvariablesmember=temp_customvariablesmember->next){
			if(temp_customvariablesmember->variable_name)
				fprintf(fp,"\t_%s=%d;%s\n",temp_customvariablesmember->variable_name,temp_customvariablesmember->has_been_modified,(temp_customvariablesmember->variable_value==NULL)?"":temp_customvariablesmember->variable_value);
		        }
		fprintf(fp,"\t}\n\n");
	        }

	/* save contact status data */
	for(temp_contact=contact_list;temp_contact!=NULL;temp_contact=temp_contact->next){

		fprintf(fp,"contact {\n");
		fprintf(fp,"\tcontact_name=%s\n",temp_contact->name);
		fprintf(fp,"\tmodified_attributes=%lu\n",temp_contact->modified_attributes);
		fprintf(fp,"\tmodified_host_attributes=%lu\n",temp_contact->modified_host_attributes);
		fprintf(fp,"\tmodified_service_attributes=%lu\n",temp_contact->modified_service_attributes);
		fprintf(fp,"\tlast_host_notification=%lu\n",temp_contact->last_host_notification);
		fprintf(fp,"\tlast_service_notification=%lu\n",temp_contact->last_service_notification);
		fprintf(fp,"\thost_notifications_enabled=%d\n",temp_contact->host_notifications_enabled);
		fprintf(fp,"\tservice_notifications_enabled=%d\n",temp_contact->service_notifications_enabled);
		/* custom variables */
		for(temp_customvariablesmember=temp_contact->custom_variables;temp_customvariablesmember!=NULL;temp_customvariablesmember=temp_customvariablesmember->next){
			if(temp_customvariablesmember->variable_name)
				fprintf(fp,"\t_%s=%d;%s\n",temp_customvariablesmember->variable_name,temp_customvariablesmember->has_been_modified,(temp_customvariablesmember->variable_value==NULL)?"":temp_customvariablesmember->variable_value);
		        }
		fprintf(fp,"\t}\n\n");
	        }


	/* reset file permissions */
	fchmod(fd,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

	/* close the temp file */
	fclose(fp);

	/* move the temp file to the status log (overwrite the old status log) */
	if(my_rename(temp_file,xsddefault_status_log)){

		/* log an error */
		asprintf(&temp_buffer,"Error: Unable to update status data file '%s'!\n",xsddefault_status_log);
		write_to_logs_and_console(temp_buffer,NSLOG_RUNTIME_ERROR,TRUE);
		my_free((void **)&temp_buffer);

		/* free memory */
		my_free((void **)&temp_file);

		return ERROR;
	        }

	/* free memory */
	my_free((void **)&temp_file);

	return OK;
        }

#endif



#ifdef NSCGI

/******************************************************************/
/****************** DEFAULT DATA INPUT FUNCTIONS ******************/
/******************************************************************/

/* read all program, host, and service status information */
int xsddefault_read_status_data(char *config_file,int options){
	char *input=NULL;
	mmapfile *thefile;
	int data_type=XSDDEFAULT_NO_DATA;
	hoststatus *temp_hoststatus=NULL;
	servicestatus *temp_servicestatus=NULL;
	char *var;
	char *val;
	int result;

	/* grab configuration data */
	result=xsddefault_grab_config_info(config_file);
	if(result==ERROR)
		return ERROR;

	/* open the status file for reading */
	if((thefile=mmap_fopen(xsddefault_status_log))==NULL)
		return ERROR;

	/* read all lines in the status file */
	while(1){

		/* free memory */
		my_free((void **)&input);

		/* read the next line */
		if((input=mmap_fgets(thefile))==NULL)
			break;

		strip(input);

		/* skip blank lines and comments */
		if(input[0]=='#' || input[0]=='\x0')
			continue;

		else if(!strcmp(input,"info {"))
			data_type=XSDDEFAULT_INFO_DATA;
		else if(!strcmp(input,"program {"))
			data_type=XSDDEFAULT_PROGRAM_DATA;
		else if(!strcmp(input,"host {")){
			data_type=XSDDEFAULT_HOST_DATA;
			temp_hoststatus=(hoststatus *)malloc(sizeof(hoststatus));
			if(temp_hoststatus){
				temp_hoststatus->host_name=NULL;
				temp_hoststatus->plugin_output=NULL;
				temp_hoststatus->long_plugin_output=NULL;
				temp_hoststatus->perf_data=NULL;
			        }
		        }
		else if(!strcmp(input,"service {")){
			data_type=XSDDEFAULT_SERVICE_DATA;
			temp_servicestatus=(servicestatus *)malloc(sizeof(servicestatus));
			if(temp_servicestatus){
				temp_servicestatus->host_name=NULL;
				temp_servicestatus->description=NULL;
				temp_servicestatus->plugin_output=NULL;
				temp_servicestatus->long_plugin_output=NULL;
				temp_servicestatus->perf_data=NULL;
			        }
		        }
		else if(!strcmp(input,"contact {")){
			data_type=XSDDEFAULT_CONTACT_DATA;
			/* unimplemented */
		        }

		else if(!strcmp(input,"}")){

			switch(data_type){

			case XSDDEFAULT_INFO_DATA:
				break;

			case XSDDEFAULT_PROGRAM_DATA:
				break;

			case XSDDEFAULT_HOST_DATA:
				add_host_status(temp_hoststatus);
				temp_hoststatus=NULL;
				break;

			case XSDDEFAULT_SERVICE_DATA:
				add_service_status(temp_servicestatus);
				temp_servicestatus=NULL;
				break;

			case XSDDEFAULT_CONTACT_DATA:
				/* unimplemented */
				break;

			default:
				break;
			        }

			data_type=XSDDEFAULT_NO_DATA;
		        }

		else if(data_type!=XSDDEFAULT_NO_DATA){

			var=strtok(input,"=");
			val=strtok(NULL,"\n");
			if(val==NULL)
				continue;

			switch(data_type){

			case XSDDEFAULT_INFO_DATA:
				break;

			case XSDDEFAULT_PROGRAM_DATA:
				/* NOTE: some vars are not read, as they are not used by the CGIs (modified attributes, event handler commands, etc.) */
				if(!strcmp(var,"nagios_pid"))
					nagios_pid=atoi(val);
				else if(!strcmp(var,"daemon_mode"))
					daemon_mode=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"program_start"))
					program_start=strtoul(val,NULL,10);
				else if(!strcmp(var,"last_command_check"))
					last_command_check=strtoul(val,NULL,10);
				else if(!strcmp(var,"last_log_rotation"))
					last_log_rotation=strtoul(val,NULL,10);
				else if(!strcmp(var,"enable_notifications"))
					enable_notifications=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"active_service_checks_enabled"))
					execute_service_checks=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"passive_service_checks_enabled"))
					accept_passive_service_checks=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"active_host_checks_enabled"))
					execute_host_checks=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"passive_host_checks_enabled"))
					accept_passive_host_checks=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"enable_event_handlers"))
					enable_event_handlers=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"obsess_over_services"))
					obsess_over_services=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"obsess_over_hosts"))
					obsess_over_hosts=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"check_service_freshness"))
					check_service_freshness=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"check_host_freshness"))
					check_host_freshness=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"enable_flap_detection"))
					enable_flap_detection=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"enable_failure_prediction"))
					enable_failure_prediction=(atoi(val)>0)?TRUE:FALSE;
				else if(!strcmp(var,"process_performance_data"))
					process_performance_data=(atoi(val)>0)?TRUE:FALSE;
				break;

			case XSDDEFAULT_HOST_DATA:
				/* NOTE: some vars are not read, as they are not used by the CGIs (modified attributes, event handler commands, etc.) */
				if(temp_hoststatus!=NULL){
					if(!strcmp(var,"host_name"))
						temp_hoststatus->host_name=(char *)strdup(val);
					else if(!strcmp(var,"has_been_checked"))
						temp_hoststatus->has_been_checked=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"should_be_scheduled"))
						temp_hoststatus->should_be_scheduled=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"check_execution_time"))
						temp_hoststatus->execution_time=strtod(val,NULL);
					else if(!strcmp(var,"check_latency"))
						temp_hoststatus->latency=strtod(val,NULL);
					else if(!strcmp(var,"check_type"))
						temp_hoststatus->check_type=atoi(val);
					else if(!strcmp(var,"current_state"))
						temp_hoststatus->status=atoi(val);
					else if(!strcmp(var,"last_hard_state"))
						temp_hoststatus->last_hard_state=atoi(val);
					else if(!strcmp(var,"plugin_output"))
						temp_hoststatus->plugin_output=(char *)strdup(val);
					else if(!strcmp(var,"long_plugin_output"))
						temp_hoststatus->long_plugin_output=(char *)strdup(val);
					else if(!strcmp(var,"performance_data"))
						temp_hoststatus->perf_data=(char *)strdup(val);
					else if(!strcmp(var,"current_attempt"))
						temp_hoststatus->current_attempt=atoi(val);
					else if(!strcmp(var,"max_attempts"))
						temp_hoststatus->max_attempts=atoi(val);
					else if(!strcmp(var,"last_check"))
						temp_hoststatus->last_check=strtoul(val,NULL,10);
					else if(!strcmp(var,"next_check"))
						temp_hoststatus->next_check=strtoul(val,NULL,10);
					else if(!strcmp(var,"current_attempt"))
						temp_hoststatus->current_attempt=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"state_type"))
						temp_hoststatus->state_type=atoi(val);
					else if(!strcmp(var,"last_state_change"))
						temp_hoststatus->last_state_change=strtoul(val,NULL,10);
					else if(!strcmp(var,"last_hard_state_change"))
						temp_hoststatus->last_hard_state_change=strtoul(val,NULL,10);
					else if(!strcmp(var,"last_time_up"))
						temp_hoststatus->last_time_up=strtoul(val,NULL,10);
					else if(!strcmp(var,"last_time_down"))
						temp_hoststatus->last_time_down=strtoul(val,NULL,10);
					else if(!strcmp(var,"last_time_unreachable"))
						temp_hoststatus->last_time_unreachable=strtoul(val,NULL,10);
					else if(!strcmp(var,"last_notification"))
						temp_hoststatus->last_notification=strtoul(val,NULL,10);
					else if(!strcmp(var,"next_notification"))
						temp_hoststatus->next_notification=strtoul(val,NULL,10);
					else if(!strcmp(var,"no_more_notifications"))
						temp_hoststatus->no_more_notifications=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"current_notification_number"))
						temp_hoststatus->current_notification_number=atoi(val);
					else if(!strcmp(var,"notifications_enabled"))
						temp_hoststatus->notifications_enabled=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"problem_has_been_acknowledged"))
						temp_hoststatus->problem_has_been_acknowledged=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"acknowledgement_type"))
						temp_hoststatus->acknowledgement_type=atoi(val);
					else if(!strcmp(var,"active_checks_enabled"))
						temp_hoststatus->checks_enabled=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"passive_checks_enabled"))
						temp_hoststatus->accept_passive_host_checks=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"event_handler_enabled"))
						temp_hoststatus->event_handler_enabled=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"flap_detection_enabled"))
						temp_hoststatus->flap_detection_enabled=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"failure_prediction_enabled"))
						temp_hoststatus->failure_prediction_enabled=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"process_performance_data"))
						temp_hoststatus->process_performance_data=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"obsess_over_host"))
						temp_hoststatus->obsess_over_host=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"last_update"))
						temp_hoststatus->last_update=strtoul(val,NULL,10);
					else if(!strcmp(var,"is_flapping"))
						temp_hoststatus->is_flapping=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"percent_state_change"))
						temp_hoststatus->percent_state_change=strtod(val,NULL);
					else if(!strcmp(var,"scheduled_downtime_depth"))
						temp_hoststatus->scheduled_downtime_depth=atoi(val);
					/*
					else if(!strcmp(var,"state_history")){
						temp_ptr=val;
						for(x=0;x<MAX_STATE_HISTORY_ENTRIES;x++)
							temp_hoststatus->state_history[x]=atoi(my_strsep(&temp_ptr,","));
						temp_hoststatus->state_history_index=0;
					        }
					*/
				        }
				break;

			case XSDDEFAULT_SERVICE_DATA:
				/* NOTE: some vars are not read, as they are not used by the CGIs (modified attributes, event handler commands, etc.) */
				if(temp_servicestatus!=NULL){
					if(!strcmp(var,"host_name"))
						temp_servicestatus->host_name=(char *)strdup(val);
					else if(!strcmp(var,"service_description"))
						temp_servicestatus->description=(char *)strdup(val);
					else if(!strcmp(var,"has_been_checked"))
						temp_servicestatus->has_been_checked=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"should_be_scheduled"))
						temp_servicestatus->should_be_scheduled=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"check_execution_time"))
						temp_servicestatus->execution_time=strtod(val,NULL);
					else if(!strcmp(var,"check_latency"))
						temp_servicestatus->latency=strtod(val,NULL);
					else if(!strcmp(var,"check_type"))
						temp_servicestatus->check_type=atoi(val);
					else if(!strcmp(var,"current_state"))
						temp_servicestatus->status=atoi(val);
					else if(!strcmp(var,"last_hard_state"))
						temp_servicestatus->last_hard_state=atoi(val);
					else if(!strcmp(var,"current_attempt"))
						temp_servicestatus->current_attempt=atoi(val);
					else if(!strcmp(var,"max_attempts"))
						temp_servicestatus->max_attempts=atoi(val);
					else if(!strcmp(var,"state_type"))
						temp_servicestatus->state_type=atoi(val);
					else if(!strcmp(var,"last_state_change"))
						temp_servicestatus->last_state_change=strtoul(val,NULL,10);
					else if(!strcmp(var,"last_hard_state_change"))
						temp_servicestatus->last_hard_state_change=strtoul(val,NULL,10);
					else if(!strcmp(var,"last_time_ok"))
						temp_servicestatus->last_time_ok=strtoul(val,NULL,10);
					else if(!strcmp(var,"last_time_warning"))
						temp_servicestatus->last_time_warning=strtoul(val,NULL,10);
					else if(!strcmp(var,"last_time_unknown"))
						temp_servicestatus->last_time_unknown=strtoul(val,NULL,10);
					else if(!strcmp(var,"last_time_critical"))
						temp_servicestatus->last_time_critical=strtoul(val,NULL,10);
					else if(!strcmp(var,"plugin_output"))
						temp_servicestatus->plugin_output=(char *)strdup(val);
					else if(!strcmp(var,"long_plugin_output"))
						temp_servicestatus->long_plugin_output=(char *)strdup(val);
					else if(!strcmp(var,"performance_data"))
						temp_servicestatus->perf_data=(char *)strdup(val);
					else if(!strcmp(var,"last_check"))
						temp_servicestatus->last_check=strtoul(val,NULL,10);
					else if(!strcmp(var,"next_check"))
						temp_servicestatus->next_check=strtoul(val,NULL,10);
					else if(!strcmp(var,"current_notification_number"))
						temp_servicestatus->current_notification_number=atoi(val);
					else if(!strcmp(var,"last_notification"))
						temp_servicestatus->last_notification=strtoul(val,NULL,10);
					else if(!strcmp(var,"next_notification"))
						temp_servicestatus->next_notification=strtoul(val,NULL,10);
					else if(!strcmp(var,"no_more_notifications"))
						temp_servicestatus->no_more_notifications=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"notifications_enabled"))
						temp_servicestatus->notifications_enabled=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"active_checks_enabled"))
						temp_servicestatus->checks_enabled=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"passive_checks_enabled"))
						temp_servicestatus->accept_passive_service_checks=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"event_handler_enabled"))
						temp_servicestatus->event_handler_enabled=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"problem_has_been_acknowledged"))
						temp_servicestatus->problem_has_been_acknowledged=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"acknowledgement_type"))
						temp_servicestatus->acknowledgement_type=atoi(val);
					else if(!strcmp(var,"flap_detection_enabled"))
						temp_servicestatus->flap_detection_enabled=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"failure_prediction_enabled"))
						temp_servicestatus->failure_prediction_enabled=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"process_performance_data"))
						temp_servicestatus->process_performance_data=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"obsess_over_service"))
						temp_servicestatus->obsess_over_service=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"last_update"))
						temp_servicestatus->last_update=strtoul(val,NULL,10);
					else if(!strcmp(var,"is_flapping"))
						temp_servicestatus->is_flapping=(atoi(val)>0)?TRUE:FALSE;
					else if(!strcmp(var,"percent_state_change"))
						temp_servicestatus->percent_state_change=strtod(val,NULL);
					else if(!strcmp(var,"scheduled_downtime_depth"))
						temp_servicestatus->scheduled_downtime_depth=atoi(val);
					/*
					else if(!strcmp(var,"state_history")){
						temp_ptr=val;
						for(x=0;x<MAX_STATE_HISTORY_ENTRIES;x++)
							temp_servicestatus->state_history[x]=atoi(my_strsep(&temp_ptr,","));
						temp_servicestatus->state_history_index=0;
					        }
					*/
				        }
				break;

			case XSDDEFAULT_CONTACT_DATA:
				/* unimplemented */
				break;

			default:
				break;
			        }

		        }
	        }

	/* free memory and close the file */
	my_free((void **)&input);
	mmap_fclose(thefile);

	/* free memory */
	my_free((void **)&xsddefault_status_log);
	my_free((void **)&xsddefault_temp_file);

	return OK;
        }

#endif

