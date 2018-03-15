#ifndef _MOBCAM_IF_H_
#define _MOBCAM_IF_H_



void if_on_register_result(int comments_id, bool approved, bool allow_hide);
void if_on_register_network_error();

int if_get_audio_channels();
int if_get_video_channels();


//
//  0: OK
// -1: NG
int if_get_device_uuid(char *buff, int size);
//
//  0: OK
// -1: NG
int if_get_nodeid(char *buff, int size);
//
//  0: OK
// -1: NG
int if_read_password(char *buff, int size);//orig pass
//
//  0: OK
// -1: NG
int if_read_nodename(char *buff, int size);
//
//  0: OK
// -1: NG
int if_get_osinfo(char *buff, int size);


int  if_video_capture_start(int width, int height, int fps, int channel);
void if_video_capture_stop();
int  if_hw264_capture_start(int width, int height, int fps, int channel);
void if_hw264_capture_stop();
int  if_hw263_capture_start(int width, int height, int fps, int channel);
void if_hw263_capture_stop();
int  if_audio_record_start(int channel);
void if_audio_record_stop();

int if_sensor_capture_start();
void if_sensor_capture_stop();

void if_play_pcm_data(const BYTE *pcm_data, int len);

void if_video_switch(int param);

void if_contrl_zoom_in();
void if_contrl_zoom_out();
void if_contrl_recordvol_up();
void if_contrl_recordvol_down();
void if_contrl_recordvol_set(int param);
void if_contrl_flash_on();
void if_contrl_flash_off();

void if_contrl_left_servo(int param);
void if_contrl_right_servo(int param);

void if_contrl_take_picture();

void if_contrl_turn_up();
void if_contrl_turn_down();
void if_contrl_turn_left();
void if_contrl_turn_right();
void if_contrl_joystick1(int param);
void if_contrl_joystick2(int param);
void if_contrl_button_a();
void if_contrl_button_b();
void if_contrl_button_x();
void if_contrl_button_y();
void if_contrl_button_l1(int param);
void if_contrl_button_l2(int param);
void if_contrl_button_r1(int param);
void if_contrl_button_r2(int param);

void if_mc_arm();
void if_mc_disarm();
void if_contrl_system_reboot();
void if_contrl_system_shutdown();

void if_on_client_connected();
void if_on_client_disconnected();

BOOL if_should_do_upnp();

void if_on_mavlink_start();
void if_on_mavlink_stop();
void if_on_mavlink_guid(float lati, float longi, float alti);
int if_get_wp_data(WP_ITEM **lpItems, int *lpNum);
int if_get_tlv_data(BYTE **lpPtr, int *lpLen);

#endif /* _MOBCAM_IF_H_ */
