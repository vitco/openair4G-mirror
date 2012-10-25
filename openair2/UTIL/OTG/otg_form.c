
#include "otg_form.h"
#include "otg_vars.h"

extern unsigned char NB_eNB_INST;
extern unsigned char NB_UE_INST;

FD_otg * form_dl, *form_ul;
FL_FORM *fclock;
unsigned int clear_cmpt_ul=0;
unsigned int clear_cmpt_dl=0;

FD_otg *create_form_otg(void)
{
  FL_OBJECT *obj, *obj_ctime;
  FD_otg *fdui = (FD_otg *) fl_calloc(1, sizeof(*fdui));

  fdui->otg = fl_bgn_form(FL_NO_BOX, 550, 550);
  obj = fl_add_box(FL_UP_BOX,0,0,900,700,"");
  fdui->owd = obj = fl_add_xyplot(FL_NORMAL_XYPLOT,50,30,450,190,"One Way Delay(ms)"); 
  fl_set_object_color(obj,FL_BLACK,FL_YELLOW);
  fdui->throughput = obj = fl_add_xyplot(FL_NORMAL_XYPLOT,50,300,450,190,"Throughput(Kbit/s)");
  fl_set_object_color(obj,FL_BLACK,FL_YELLOW);
   
  
   obj = fl_add_button(FL_NORMAL_BUTTON,250,510,50,30,"Exit");
   fl_set_object_callback(obj, exit_cb, 0);

   //obj_ctime = fl_add_text(FL_NORMAL_TEXT, 150,510,50,30, "EURECOM");

  fl_end_form();
  fdui->otg->fdui = fdui;
  return fdui;   
}

//For more details about object parameters, refer to: http://xforms-toolkit.org/doc/xforms_25.html

void show_otg_form()
{
int eNB_id;
char title[255];
char *tArgv[] = { "OTG", "OTG" };
int tArgc = 2;

fl_initialize(&tArgc,tArgv,"OTG",0,0);
    for (eNB_id = 0; eNB_id < NB_eNB_INST; eNB_id++) { //NB_eNB_INST
      form_dl= create_form_otg ();
      sprintf (title, "LTE eNB->UE (DL)");
      fl_show_form (form_dl->otg, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);
      fl_set_form_position(form_dl->otg, 200, 200);
        if (g_otg->owd_radio_access==1)
          fl_set_xyplot_ybounds(form_dl->owd,0,30);
        else
          fl_set_xyplot_ybounds(form_dl->owd,0,200);
 
      fl_set_xyplot_ybounds(form_dl->throughput,0,100); 
      form_ul= create_form_otg ();
      sprintf (title, "LTE UE->eNB (UL)");
      fl_show_form (form_ul->otg, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);
      fl_set_form_position(form_ul->otg, 850, 200);
      if (g_otg->owd_radio_access==1)
          fl_set_xyplot_ybounds(form_ul->owd,0,70);
        else
          fl_set_xyplot_ybounds(form_ul->owd,0,200);
      fl_set_xyplot_ybounds(form_ul->throughput,0,50); 

    }

    //create_form_clock();
    //fl_show_form(fclock, FL_PLACE_CENTER,FL_TRANSIENT,"clocks");
    //fl_do_forms();

   fl_check_forms();
}	

																																																																																																																																																																																																		
void add_tab_metric(int src, int dst, float owd, float throughput, int ctime){

  if (otg_forms_info->init_forms==0){
    show_otg_form();
    otg_forms_info->init_forms=1;
    otg_forms_info->is_data_plot_ul=-1; 
    otg_forms_info->is_data_plot_dl=-1;
  }

if (dst<NB_eNB_INST){ 
    //LOG_D(OTG,"direction: UL [src:%d, dst:%d] \n", src, dst);
    otg_forms_info->data_owd_ul[src][dst][otg_forms_info->idx_ul[src][dst]]= owd;
    otg_forms_info->data_throughput_ul[src][dst][otg_forms_info->idx_ul[src][dst]]=throughput;
    otg_forms_info->data_ctime_ul[src][dst][otg_forms_info->idx_ul[src][dst]]= otg_forms_info->idx_ul[src][dst]; 
    otg_forms_info->idx_ul[src][dst]++;
    plot_graphes_ul(src, dst);
  }
  else{
    //LOG_D(OTG,"direction: DL [src:%d, dst:%d] \n", src, dst);
    otg_forms_info->data_owd_dl[src][dst][otg_forms_info->idx_dl[src][dst]]= owd;
    otg_forms_info->data_throughput_dl[src][dst][otg_forms_info->idx_dl[src][dst]]= throughput;
    otg_forms_info->data_ctime_dl[src][dst][otg_forms_info->idx_dl[src][dst]]= otg_forms_info->idx_dl[src][dst]; 
    otg_forms_info->idx_dl[src][dst]++;
    plot_graphes_dl(src, dst);
  }
  //LOG_D(OTG,"OTG_forms[src %d, dst %d] owd %f TH %f \n", src, dst,  owd, throughput);	
  //LOG_D(OTG,"MAX_UE_eNB %d, %d \n:",  NB_UE_INST,  NB_eNB_INST);   

//printf("otg_counter: UL %d , DL %d \n", clear_cmpt_ul, clear_cmpt_dl);
}


void plot_graphes_ul(int src, int dst) //UE -->eNB
{

int i, src_idx=1, curve_id=1;

if (otg_forms_info->idx_ul[src][dst]==MAX_SAMPLES-1){

  fl_update_display(1); //the function flushes the X buffer so the drawing requests are on their way to the server


  if (otg_forms_info->is_data_plot_ul == -1) 
    otg_forms_info->is_data_plot_ul=src;

  if (otg_forms_info->is_data_plot_ul == src) {
    fl_set_xyplot_data (form_ul->owd, otg_forms_info->data_ctime_ul[src][dst],
    otg_forms_info->data_owd_ul[src][dst], otg_forms_info->idx_ul[src][dst], "", "time", "ms");  

    fl_set_xyplot_data (form_ul->throughput, otg_forms_info->data_ctime_ul[src][dst],
    otg_forms_info->data_throughput_ul[src][dst], otg_forms_info->idx_ul[src][dst], "", "time", "Kbit/s");  
  } 
  else {
    fl_set_xyplot_data (form_ul->owd, otg_forms_info->data_ctime_ul[otg_forms_info->is_data_plot_ul][dst],
    otg_forms_info->data_owd_ul[otg_forms_info->is_data_plot_ul][dst], otg_forms_info->idx_ul[otg_forms_info->is_data_plot_ul][dst], "", "time", "ms");  
    fl_set_xyplot_data (form_ul->throughput, otg_forms_info->data_ctime_ul[otg_forms_info->is_data_plot_ul][dst],
    otg_forms_info->data_throughput_ul[otg_forms_info->is_data_plot_ul][dst], otg_forms_info->idx_ul[otg_forms_info->is_data_plot_ul][dst], "", "time", "Kbit/s");  

  }

  for (src_idx=1;src_idx<=NB_UE_INST;src_idx++){
    if (src_idx!=otg_forms_info->is_data_plot_ul){
      fl_add_xyplot_overlay(form_ul->owd,curve_id++,
      otg_forms_info->data_ctime_ul[src_idx][dst],
      otg_forms_info->data_owd_ul[src_idx][dst],
      otg_forms_info->idx_ul[src_idx][dst],src_idx+6);

      fl_add_xyplot_overlay(form_ul->throughput,curve_id++,
      otg_forms_info->data_ctime_ul[src_idx][dst],
      otg_forms_info->data_throughput_ul[src_idx][dst],
      otg_forms_info->idx_ul[src_idx][dst],src_idx+6);

    } 
  }

  for (i=0;i<otg_forms_info->idx_ul[src][dst];i++){
    otg_forms_info->data_ctime_ul[src][dst][otg_forms_info->idx_ul[src][dst]]=i;
    otg_forms_info->data_owd_ul[src][dst][i]= otg_forms_info->data_owd_ul[src][dst][i+1];
    otg_forms_info->data_throughput_ul[src][dst][i]= otg_forms_info->data_throughput_ul[src][dst][i+1];
  }
  otg_forms_info->idx_ul[src][dst]--;
}

   fl_check_forms();

}



void plot_graphes_dl(int src, int dst)  //eNB -->UE
{

int i, dst_idx=1, curve_id=1;

if (otg_forms_info->idx_dl[src][dst]==MAX_SAMPLES-1){

  fl_update_display(1); //the function flushes the X buffer so the drawing requests are on their way to the server


  if (otg_forms_info->is_data_plot_dl == -1) 
    otg_forms_info->is_data_plot_dl=dst;

  if (otg_forms_info->is_data_plot_dl == dst) {
    fl_set_xyplot_data (form_dl->owd, otg_forms_info->data_ctime_dl[src][dst],
    otg_forms_info->data_owd_dl[src][dst], otg_forms_info->idx_dl[src][dst], "", "time", "ms");   

    fl_set_xyplot_data (form_dl->throughput, otg_forms_info->data_ctime_dl[src][dst],
    otg_forms_info->data_throughput_dl[src][dst], otg_forms_info->idx_dl[src][dst], "", "time", "Kbit/s");  
//printf("DL_owd (1)[src %d,  dst %d] owd %f, idx %d \n", src, dst, otg_forms_info->data_owd_dl[src][dst][10], otg_forms_info->idx_dl[src][dst] );
  } 
  else {
    fl_set_xyplot_data (form_dl->owd, otg_forms_info->data_ctime_dl[src][otg_forms_info->is_data_plot_dl],
    otg_forms_info->data_owd_dl[src][otg_forms_info->is_data_plot_dl], otg_forms_info->idx_dl[src][otg_forms_info->is_data_plot_dl], "", "time", "ms");  

    fl_set_xyplot_data (form_dl->throughput, otg_forms_info->data_ctime_dl[src][otg_forms_info->is_data_plot_dl],
    otg_forms_info->data_throughput_dl[src][otg_forms_info->is_data_plot_dl], otg_forms_info->idx_dl[src][otg_forms_info->is_data_plot_dl], "", "time", "Kbit/s");  

  }

  for (dst_idx=1;dst_idx<=NB_UE_INST;dst_idx++){
    if (dst_idx!=otg_forms_info->is_data_plot_dl){
      fl_add_xyplot_overlay(form_dl->owd,curve_id++,
      otg_forms_info->data_ctime_dl[src][dst_idx],
      otg_forms_info->data_owd_dl[src][dst_idx],
      otg_forms_info->idx_dl[src][dst_idx],dst_idx+6);

      fl_add_xyplot_overlay(form_dl->throughput,curve_id++,
      otg_forms_info->data_ctime_dl[src][dst_idx],
      otg_forms_info->data_throughput_dl[src][dst_idx],
      otg_forms_info->idx_dl[src][dst_idx],dst_idx+6);


    } 
  }

  for (i=0;i<otg_forms_info->idx_dl[src][dst];i++){
    otg_forms_info->data_ctime_dl[src][dst][otg_forms_info->idx_dl[src][dst]]=i;
    otg_forms_info->data_owd_dl[src][dst][i]= otg_forms_info->data_owd_dl[src][dst][i+1];
    otg_forms_info->data_throughput_dl[src][dst][i]= otg_forms_info->data_throughput_dl[src][dst][i+1];
  }
  otg_forms_info->idx_dl[src][dst]--;
}

   fl_check_forms();

}






void exit_cb(FL_OBJECT *ob, long q)
{
   fl_finish();
   exit(0);
}

void create_form_clock(void)
{
   FL_OBJECT *obj;

   if (fclock)
       return;

   fclock = fl_bgn_form(FL_NO_BOX,500,350);
   obj = fl_add_box(FL_UP_BOX,0,0,500,350,"");

   obj = fl_add_clock(FL_DIGITAL_CLOCK,185,20,150,35,"");
   fl_set_object_boxtype(obj,FL_ROUNDED_BOX); 
   fl_set_object_color(obj,FL_COL1,FL_BLACK);
   fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
   fl_set_object_lstyle(obj,FL_BOLD_STYLE);

   obj = fl_add_clock(FL_ANALOG_CLOCK,30,70,220,200,"");
   fl_set_object_boxtype(obj,FL_UP_BOX);

   obj = fl_add_clock(FL_ANALOG_CLOCK,260,70,220,200,"");
   fl_set_object_boxtype(obj,FL_OVAL3D_UPBOX);
   obj = fl_add_button(FL_NORMAL_BUTTON,375,300,110,35,"Exit");
   fl_set_object_callback(obj, exit_cb, 0);
   fl_end_form();

   fl_scale_form(fclock, 0.7, 0.7);
}



