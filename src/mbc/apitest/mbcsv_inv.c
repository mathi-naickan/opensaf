#include <unistd.h>
#include <fcntl.h>
#include  "tet_api.h"
#include "mbcsv_api.h"

typedef struct mbcstm_file
{
  uint32_t fd;
  uint32_t loc;
} MBCSTM_FILE;

static uint32_t mbcstm_get_output_descriptor(MBCSTM_FILE *fd,MBCSTM_FILE *out_fd);
static uint32_t mbcstm_close_output_descriptor(MBCSTM_FILE *fd,
                                            MBCSTM_FILE *out_fd);
uint32_t mbcstm_get_info(MBCSTM_FILE *fd,MBCSTM_CHECK check, uint32_t svc_index, 
                      uint32_t ssn_index, void *data);
static uint32_t mbcstm_header_compare(MBCSTM_FILE *fd);
static uint32_t mbcstm_footer_compare(MBCSTM_FILE *fd);
static int mbcstm_getline(MBCSTM_FILE *fd,char *buf);
static int mbcstm_line_compare(MBCSTM_FILE *fd,char *buf);
uint32_t mbcstm_goto_header(MBCSTM_FILE *fd);

static uint32_t mbcstm_get_output_descriptor(MBCSTM_FILE *fd,MBCSTM_FILE *out_fd)
{
  char str[256];

  /* make file string */
  sprintf(str,"/tmp/mbcsv_%d_%d.txt",mbcstm_cb.sys,case_num);
  /* unlink(str);  */

  /* dup STDOUT in to out fd */
  if((out_fd->fd = fcntl(1,F_DUPFD,0)) <  0)
    return NCSCC_RC_FAILURE;

  /* close STDOUT */
  close(1);

  /* open file for fd of STDOUT */ 
  if((fd->fd = open(str,O_CREAT | O_TRUNC | O_APPEND | O_RDWR)) < 0)
    return NCSCC_RC_FAILURE;

  if(fd->fd  != 1)
    return NCSCC_RC_FAILURE;

  return NCSCC_RC_SUCCESS;

}

static uint32_t mbcstm_close_output_descriptor(MBCSTM_FILE *fd,
                                            MBCSTM_FILE *out_fd)
{
  char str[256];

  /* make file string */
  sprintf(str,"/tmp/mbcsv_%d_%d.txt",mbcstm_cb.sys,case_num);

  /* close fd */
  close(fd->fd);
  /*  unlink(str); */

  /* dup back to STDOUT using out_fd */
  if(dup2(out_fd->fd,1) < 0)
    return NCSCC_RC_FAILURE;

  return NCSCC_RC_SUCCESS;
}
#if 0
MBCSTM_FSM_STATES  mbcstm_get_state_from_name(char *string)
{
  if(strcmp("IDLE",string) == 0)
    return MBCSTM_STBY_STATE_IDLE;
  else if(strcmp("WFCS",string) == 0)
    return MBCSTM_ACT_STATE_WAIT_FOR_COLD_WARM_SYNC;
  else if(strcmp("KSIS",string) == 0)
    return MBCSTM_ACT_STATE_KEEP_STBY_IN_SYNC;
  else if(strcmp("MACT",string) == 0)
    return MBCSTM_ACT_STATE_MULTIPLE_ACTIVE;
  else if(strcmp("WTCS",string) == 0)
    return MBCSTM_STBY_STATE_WAIT_TO_COLD_SYNC;
  else if(strcmp("SISY",string) == 0)
    return MBCSTM_STBY_STATE_STEADY_IN_SYNC;
  else if(strcmp("WTWS",string) == 0)
    return MBCSTM_STBY_STATE_WAIT_TO_WARM_SYNC;
  else if(strcmp("VWSD",string) == 0)
    return MBCSTM_STBY_STATE_VERIFY_WARM_SYNC_DATA;       
  else if(strcmp("WFDR",string) == 0)
    return MBCSTM_STBY_STATE_WAIT_FOR_DATA_RESP; 
  else if(strcmp("QUIE",string) == 0)
    return MBCSTM_QUIESCED_STATE;

  return 0;
}

uint32_t mbcstm_get_role_from_name(char name)
{
  switch(name)
    {
    case 'I':
      return 0;
    case 'A':
      return 1;
    case 'S':
      return 2;
    case 'Q':
      return 3;
    }

  return NCSCC_RC_FAILURE;  
}
#endif

/* extern MBCSV_CB mbcsv_cb; */
uint32_t mbcstm_check_inv(MBCSTM_CHECK check, uint32_t svc_index, uint32_t ssn_index, 
                       void *data)
{
  char    fun_name[]  = "mbcstm_check_inv";
  MBCSTM_FILE my_fd, out_fd;

  /* get STDOUT file descriptor */
  if(mbcstm_get_output_descriptor(&my_fd,&out_fd) != NCSCC_RC_SUCCESS)
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"MBCSTM CHECK INV ", 
                   NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }

  /* printf mbcsv environment in to fd */
  if(mbcsv_prt_inv() != NCSCC_RC_SUCCESS)
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"MBCSTM CHECK INV ", 
                   NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }


  if(mbcstm_get_info(&my_fd,check,svc_index,ssn_index,data) != 
     NCSCC_RC_SUCCESS)
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"MBCSTM CHECK INV ", 
                   NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }

  /* process the data for the real output */

  /* close the file descriptor for output */
  if(mbcstm_close_output_descriptor(&my_fd,&out_fd) != NCSCC_RC_SUCCESS)
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"MBCSTM CHECK INV ", 
                   NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE; 
    }

  return NCSCC_RC_SUCCESS;
}
/*change : Needs to be modified*/
uint32_t mbcstm_get_info(MBCSTM_FILE *fd,MBCSTM_CHECK check, uint32_t svc_index, 
                      uint32_t ssn_index, void *data)
{
  MBCSTM_PEERS_DATA *peers = (MBCSTM_PEERS_DATA *)data;
  char  buf[256];
  uint32_t svc_id, to_svc_id;
  uint64_t to_anchor;
  uint32_t len,inc = 1;
  uint32_t ssn_count, peer_count;
  uint32_t get_peer_info;
  char is_incompatible,is_cold_sync_done;
  uint32_t peer_incompatible,warm_sync_sent,peer_disc,ckpt_msg_sent,
    okay_to_async_updt,okay_to_send_ntfy, ack_rcvd,cold_sync_done,
    data_resp_process,c_sync_resp_process,w_syn_resp_process,
    version_relasecode,version_majorversion,version_minorversion;

  /* get the service id from control block */
  to_svc_id = mbcstm_cb.svces[svc_index].svc_id;
  to_anchor = mbcstm_cb.svces[svc_index].ssns[ssn_index].anchor;

  /* get location in to first */
  fd->loc = lseek(fd->fd,0,SEEK_SET);

  if(mbcstm_header_compare(fd) != NCSCC_RC_SUCCESS)
    {
      printf("\n header itself is not matching,some thing wrong ?");
      return NCSCC_RC_FAILURE;
    }

  if(mbcstm_footer_compare(fd) == NCSCC_RC_SUCCESS)
    {
      printf("\n footer also mathed, there is no data, please check");
      return NCSCC_RC_SUCCESS;
    }

  /* for session count zero */
  ssn_count = 0;
  peer_count = 0;

  /* 
     if(mbcstm_line_compare(fd,"|               |           NONE        |           NONE                   |") == 0)
     {
     printf("\n there are no sessions running to go further");
     mbcstm_getline(fd,*buf);
     inc = 0;
     } 
  */

  while(inc)
    {
      uint32_t hdl;
      printf("\n parsing for required service ");
      len = mbcstm_getline(fd,buf);
      if(sscanf(buf,"\n|%6d|%8X|         MY  CSI       |                                  |",&svc_id,&hdl) != 2)
        break;

      if(svc_id == to_svc_id)
        {
          uint32_t pwe_hdl,ckpt_hdl;
          char warm_sync_on;
          char role;

          uint64_t my_anchor;
          uint32_t in_quiescing,peer_up_sent,ftm_role_set,role_set,data_req_sent;
          uint32_t peer = 1;
          uint32_t ssn = 1;

          inc = 0;
          printf("sevice id matched going to  get session information");

          if(mbcstm_line_compare(fd,"|               |           NONE        |           NONE                   |") == 0)
            {
              printf("\n there are no sessions running to go further");
              break;
            }  

          /* Parse session by session and get results */
          while(ssn)
            {
              uint32_t active_peer;

              len = mbcstm_getline(fd,buf);
              if(sscanf(buf,"|               |%8X|%8X| %c |%c|                                  |",
                        &pwe_hdl,&ckpt_hdl,&warm_sync_on,&role) == 4)
                {
                  len = mbcstm_getline(fd,buf);
                  if(sscanf(buf,"|               |%16llX|%1d%1d%1d%1d%1d |           MY   PEERS              |",
                            &my_anchor,&in_quiescing,&peer_up_sent,
                            &ftm_role_set,&role_set,&data_req_sent) == 6)
                    {
                      printf("\n My cis data");
                    }              
                }
              else
                {
                  printf("\n My csi is not matching, some thing wrong");
                  return NCSCC_RC_SUCCESS;
                }

              ssn_count++;
              if(my_anchor == to_anchor)
                get_peer_info = 1;

              printf("\n I got my csi, going to get peers");
              if(mbcstm_line_compare(fd,"|               |                       |           NONE                   |") == 0)           
                {
                  printf("\nit seems there are no peers at present");
                  peer = 0;
                  peer_count = 0;
                  if(mbcstm_line_compare(fd,"|               |-----------------------|----------------------------------|") == 0)
                    {
                      printf("\n end reached");fflush(stdout);
                      return NCSCC_RC_SUCCESS; /*change*/
                    }       
                }

              while(peer)
                {
                  uint64_t peer_anchor;
                  uint32_t hdl,peer_hdl;
                  char peer_role,peer_state[5],peer_comp[2],peer_cold[2];

                  peer_state[4] = '\0';
                  peer_comp[1] = '\0';
                  peer_cold[1] = '\0';

                  len = mbcstm_getline(fd,buf);
                  if(sscanf(buf,"|               |                       |%16llX|%8X|%8X|  |",&peer_anchor,&hdl,&peer_hdl)==3)
                    {
                      len = mbcstm_getline(fd,buf);
                      if(sscanf(buf,"|               |                       |%c|%4s|%c| %c |%1d%1d%1d%1d%1d%1d%1d%1d%1d%1d%1d|%2d%2d%2d |    |",&peer_role,
                                peer_state,&is_incompatible,&is_cold_sync_done,
                                &peer_incompatible,&warm_sync_sent,&peer_disc,
                                &ckpt_msg_sent,&okay_to_async_updt,
                                &okay_to_send_ntfy,&ack_rcvd,&cold_sync_done,
                                &data_resp_process,&c_sync_resp_process,
                                &w_syn_resp_process,&version_relasecode,
                                &version_majorversion,&version_minorversion)
                         ==18)
                        {
                          printf("\n got peer data %c %s",peer_role,peer_state);
                        }
                    }
                  else
                    {
                      return NCSCC_RC_FAILURE;
                    }     

                  peer_count++;

                  if(get_peer_info == 1)
                    {
                      peers->peer_count++;       
                      /*
                        peers->peers[peer_count].state = 
                        mbcstm_get_state_from_name(peer_state);
                      strcpy(peers->peers[peer_count].state,peer_state);
                      peers->peers[peer_count].peer_anchor = peer_anchor; 
                      */
                      /*
                        peers->peers[peer_count].peer_role = 
                        mbcstm_get_role_from_name(peer_role); 
                      */
                      peers->peers[peer_count].peer_role=peer_role;
                      /*
                        peers->peers[peer_count].is_incompatible=is_incompatible;
                        peers->peers[peer_count].is_cold_sync_done=is_cold_sync_done;
                        peers->peers[peer_count].peer_incompatible=peer_incompatible;
                        peers->peers[peer_count].warm_sync_sent=warm_sync_sent;
                        peers->peers[peer_count].peer_disc=peer_disc;
                        peers->peers[peer_count].ckpt_msg_sent=ckpt_msg_sent;
                        peers->peers[peer_count].okay_to_async_updt=okay_to_async_updt;
                        peers->peers[peer_count].okay_to_send_ntfy=okay_to_send_ntfy;
                        peers->peers[peer_count].ack_rcvd=ack_rcvd;
                        peers->peers[peer_count].cold_sync_done=cold_sync_done;
                        peers->peers[peer_count].data_resp_process=data_resp_process;
                        peers->peers[peer_count].c_sync_resp_process=c_sync_resp_process;
                        peers->peers[peer_count].w_syn_resp_process=w_syn_resp_process;
                      */
                    }

                  if(mbcstm_line_compare(fd,"|               |                       |           MY ACTIVE PEER         |") == 0)
                    {
                      printf("\n There is active peers, end of normal peers");
                      active_peer = 1;
                      break;
                    }
                  else
                    {
                      if(mbcstm_line_compare(fd,"|               |-----------------------|----------------------------------|") == 0)
                        {
                          printf("\n end of peers");
                          active_peer = 0;
                          break;
                        }
                    }     

                } /* end of while(peers) */

              get_peer_info = 0; 
              if(active_peer)        
                {
                  uint64_t peer_anchor;
                  uint32_t hdl,peer_hdl;
                  char peer_role,peer_state[8];

                  len = mbcstm_getline(fd,buf);
                  if(sscanf(buf,"|               |                       |%16llX|%8X|%8X|  |",&peer_anchor,&hdl,&peer_hdl)==3)            
                    /*
                      if(sscanf(buf,"|               |                       |%3d|%c|%4s|%1s| %1s |%8X|%8X|",
                      &peer_anchor,&peer_role,
                      peer_state,peer_comp,peer_cold,
                      &hdl, &peer_hdl) == 7)
                    */
                    {
                      len = mbcstm_getline(fd,buf);
                      if(sscanf(buf,"|               |                       |%c|%4s|%c| %c |%1d%1d%1d%1d%1d%1d%1d%1d%1d%1d%1d|%2d%2d%2d |    |",&peer_role,
                                peer_state,&is_incompatible,&is_cold_sync_done,
                                &peer_incompatible,&warm_sync_sent,&peer_disc,
                                &ckpt_msg_sent,&okay_to_async_updt,
                                &okay_to_send_ntfy,&ack_rcvd,&cold_sync_done,
                                &data_resp_process,&c_sync_resp_process,
                                &w_syn_resp_process,&version_relasecode,
                                &version_majorversion,&version_minorversion)
                         ==18)
                        {            
                          printf("\n got active peer data");
                          /*
                            peers->peers[0].state = 
                            mbcstm_get_state_from_name(peer_state);
                          strcpy(peers->peers[0].state,peer_state);
                          peers->peers[0].peer_anchor = peer_anchor; 
                          */
                          /*
                            peers->peers[0].peer_role = 
                            mbcstm_get_role_from_name(peer_role); 
                          */
                          peers->peers[0].peer_role=peer_role;
                        }
                    }
                  else
                    {
                      return NCSCC_RC_FAILURE;
                    }

                  if(mbcstm_line_compare(fd,"|               |-----------------------|----------------------------------|") == 0)
                    {
                      printf("\n end of active peer also peers");
                    }
                }
              else
                {
                  printf("\n no active peer in this ssn"); 
                }


              if(mbcstm_line_compare(fd,"------------------------------------------------------------------------------") == 0)
                {
                  printf("\n There are no ssns further ");
                  break;
                }

            }   /* end of while(ssn) */

          break;
        } /* end of if MY CSI */
    }  /* end of while(inc) */

  mbcstm_line_compare(fd,"------------------------------------------------------------------------------");
  mbcstm_footer_compare(fd);

  return NCSCC_RC_SUCCESS;
}

static uint32_t mbcstm_header_compare(MBCSTM_FILE *fd)
{
  /*
    if(mbcstm_line_compare(fd,"") != 0)
    return NCSCC_RC_FAILURE;
  */
  if(mbcstm_goto_header(fd) != NCSCC_RC_SUCCESS)
    return NCSCC_RC_FAILURE;
  if(mbcstm_line_compare(fd,"------------------------------------------------------------------------------")!=0)
    return NCSCC_RC_FAILURE;
  if(mbcstm_line_compare(fd,"|             M  B  C  S  V     I  n  v  e  n  t  o  r  y                     |")!=0)
    return NCSCC_RC_FAILURE;
  if(mbcstm_line_compare(fd,"------------------------------------------------------------------------------")!=0)
    return NCSCC_RC_FAILURE;
  if(mbcstm_line_compare(fd,"|       SVC     |          CSI          |               PEER's                |")!=0)
    return NCSCC_RC_FAILURE;
  if(mbcstm_line_compare(fd,"------------------------------------------------------------------------------")!=0)
    return NCSCC_RC_FAILURE;
  if(mbcstm_line_compare(fd,"|SVC_ID|MBCSVHDL| PWEHDL | CSIHDL | W |R|    My Anchor   | My hdl |peer hdl|  |")!=0)
    return NCSCC_RC_FAILURE;

  if(mbcstm_line_compare(fd,"|---------------|    My Anchor   |ipfrd |R|FSMS|C|SYN|iwpcooacdcw|Version|    |")!=0)
    return NCSCC_RC_FAILURE;    

  if(mbcstm_line_compare(fd,"------------------------------------------------------------------------------")!=0)
    return NCSCC_RC_FAILURE;

  return NCSCC_RC_SUCCESS;
}

uint32_t mbcstm_goto_header(MBCSTM_FILE *fd)
{
  int loca,rc;

  char buf[256];
  char *str1="------------------------------------------------------------------------------";
  char *str2="|             M  B  C  S  V     I  n  v  e  n  t  o  r  y                     |";

  while(mbcstm_line_compare(fd,"") == 0);

  while(1)
    {
      loca = fd->loc;
      if(mbcstm_getline(fd,buf) == 0)
        return NCSCC_RC_FAILURE;

      rc = strcmp(str1,buf);
      if(rc != 0)
        continue;
      if(mbcstm_getline(fd,buf) == 0)
        return NCSCC_RC_FAILURE;

      rc = strcmp(str2,buf);
      if(rc == 0)
        {
          fd->loc = loca;
          break;
        }
    }

  return NCSCC_RC_SUCCESS;
}

static uint32_t mbcstm_footer_compare(MBCSTM_FILE *fd)
{
  if(mbcstm_line_compare(fd,"------------------------------------------------------------------------------")!=0) 
    return NCSCC_RC_FAILURE; 
  if(mbcstm_line_compare(fd,"------------------------------------------------------------------------------")!=0)
    return NCSCC_RC_FAILURE; 

  return NCSCC_RC_SUCCESS;
}

static int mbcstm_getline(MBCSTM_FILE *fd,char *buf)
{
  char c = '\0';
  int i = 0;

  fd->loc = lseek(fd->fd,fd->loc,SEEK_SET);

  while(read(fd->fd,&c,1) != 0)
    {
      if(c == '\n')
        break;
      buf[i] = c;
      i++;
    }

  fd->loc = lseek(fd->fd,fd->loc+i+1,SEEK_SET);

  buf[i] = '\0';
  return i;
}

static int mbcstm_line_compare(MBCSTM_FILE *fd, char *s)
{
  int len,loca,rc;

  char buf[256];
  loca = fd->loc;

  len = mbcstm_getline(fd,buf);

  if(len == 0)
    printf("\n end of file reached ");

  /* add lseek thing in to this function */
  rc = strcmp(s,buf);

  if(rc != 0)
    fd->loc = loca;

  return(rc);
}

