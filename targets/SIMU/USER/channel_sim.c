#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "MAC_INTERFACE/extern.h"

#ifdef OPENAIR2
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log_if.h"
#include "UTIL/LOG/log_extern.h"
#include "RRC/LITE/extern.h"
#include "PHY_INTERFACE/extern.h"
#include "UTIL/OCG/OCG.h"
#include "UTIL/OPT/opt.h" // to test OPT
#endif

#include "ARCH/CBMIMO1/DEVICE_DRIVER/extern.h"

#include "UTIL/FIFO/types.h"

#ifdef IFFT_FPGA
#include "PHY/LTE_REFSIG/mod_table.h"
#endif

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#ifdef XFORMS
#include "forms.h"
#include "phy_procedures_sim_form.h"
#endif

#include "oaisim.h"

#define RF
//#define DEBUG_SIM

int number_rb_ul;
int first_rbUL ;

extern Signal_buffers_t *signal_buffers_g;

void do_OFDM_mod(mod_sym_t **txdataF, int32_t **txdata, uint32_t frame,uint16_t next_slot, LTE_DL_FRAME_PARMS *frame_parms) {

  int aa, slot_offset, slot_offset_F;

  slot_offset_F = (next_slot)*(frame_parms->ofdm_symbol_size)*((frame_parms->Ncp==1) ? 6 : 7);
  slot_offset = (next_slot)*(frame_parms->samples_per_tti>>1);
  
  for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
   if (is_pmch_subframe(frame,next_slot>>1,frame_parms)) {
      if ((next_slot%2)==0) {
	LOG_D(OCM,"Frame %d, subframe %d: Doing MBSFN modulation (slot_offset %d)\n",frame,next_slot>>1,slot_offset); 
	PHY_ofdm_mod(&txdataF[aa][slot_offset_F],        // input
		     &txdata[aa][slot_offset],         // output
		     frame_parms->log2_symbol_size,                // log2_fft_size
		     12,                 // number of symbols
		     frame_parms->ofdm_symbol_size>>2,               // number of prefix samples
		     frame_parms->twiddle_ifft,  // IFFT twiddle factors
		     frame_parms->rev,           // bit-reversal permutation
		     CYCLIC_PREFIX);
     
	if (frame_parms->Ncp == EXTENDED)
	  PHY_ofdm_mod(&txdataF[aa][slot_offset_F],        // input
		       &txdata[aa][slot_offset],         // output
		       frame_parms->log2_symbol_size,                // log2_fft_size
		       2,                 // number of symbols
		       frame_parms->nb_prefix_samples,               // number of prefix samples
		       frame_parms->twiddle_ifft,  // IFFT twiddle factors
		       frame_parms->rev,           // bit-reversal permutation
		       CYCLIC_PREFIX);
	else {
	  LOG_D(OCM,"Frame %d, subframe %d: Doing PDCCH modulation\n",frame,next_slot>>1); 
	  normal_prefix_mod(&txdataF[aa][slot_offset_F],
			    &txdata[aa][slot_offset],
			    2,
			    frame_parms);
	}      
      }
    }
    else {
      if (frame_parms->Ncp == EXTENDED)
	PHY_ofdm_mod(&txdataF[aa][slot_offset_F],        // input
		     &txdata[aa][slot_offset],         // output
		     frame_parms->log2_symbol_size,                // log2_fft_size
		     6,                 // number of symbols
		     frame_parms->nb_prefix_samples,               // number of prefix samples
		     frame_parms->twiddle_ifft,  // IFFT twiddle factors
		     frame_parms->rev,           // bit-reversal permutation
		     CYCLIC_PREFIX);
      else {
	normal_prefix_mod(&txdataF[aa][slot_offset_F],
			  &txdata[aa][slot_offset],
			  7,
			  frame_parms);
      }
    }  
  }
  
}

void do_DL_sig(double **r_re0,double **r_im0,
               double **r_re,double **r_im,
               double **s_re,double **s_im,
               channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX][MAX_NUM_CCs],
               node_desc_t *enb_data[NUMBER_OF_eNB_MAX],
               node_desc_t *ue_data[NUMBER_OF_UE_MAX],
               uint16_t next_slot,uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms,
               uint8_t UE_id,
	       int CC_id) {

  int32_t att_eNB_id=-1;
  int32_t **txdata,**rxdata;
  
  uint8_t eNB_id=0;
  double tx_pwr, rx_pwr;
  int32_t rx_pwr2;
  uint32_t i,aa;
  uint32_t slot_offset,slot_offset_meas;

  double min_path_loss=-200;
  uint8_t hold_channel=0;
  //  uint8_t aatx,aarx;
  uint8_t nb_antennas_rx = eNB2UE[0][0][CC_id]->nb_rx; // number of rx antennas at UE
  uint8_t nb_antennas_tx = eNB2UE[0][0][CC_id]->nb_tx; // number of tx antennas at eNB

  int subframe_sched = ((next_slot>>1) == 0) ? 9 : ((next_slot>>1)-1);

  
  if (next_slot==0)
    hold_channel = 0;
  else
    hold_channel = 1;

  if (abstraction_flag != 0) {
    //for (UE_id=0;UE_id<NB_UE_INST;UE_id++) {

    if (!hold_channel) {
      // calculate the random channel from each eNB
      for (eNB_id=0;eNB_id<NB_eNB_INST;eNB_id++) {
        random_channel(eNB2UE[eNB_id][UE_id][CC_id],abstraction_flag);
        /*
	for (i=0;i<eNB2UE[eNB_id][UE_id]->nb_taps;i++)
	  printf("eNB2UE[%d][%d]->a[0][%d] = (%f,%f)\n",eNB_id,UE_id,i,eNB2UE[eNB_id][UE_id]->a[0][i].x,eNB2UE[eNB_id][UE_id]->a[0][i].y);
	*/
        freq_channel(eNB2UE[eNB_id][UE_id][CC_id], frame_parms->N_RB_DL,frame_parms->N_RB_DL*12+1);
      }

      // find out which eNB the UE is attached to
      for (eNB_id=0;eNB_id<NB_eNB_INST;eNB_id++) {
        if (find_ue(PHY_vars_UE_g[UE_id][CC_id]->lte_ue_pdcch_vars[0]->crnti,PHY_vars_eNB_g[eNB_id][CC_id])>=0) {
          // UE with UE_id is connected to eNb with eNB_id
          att_eNB_id=eNB_id;
          LOG_D(OCM,"A: UE attached to eNB (UE%d->eNB%d)\n",UE_id,eNB_id);
        }
      }

      // if UE is not attached yet, find assume its the eNB with the smallest pathloss
      if (att_eNB_id<0) {
        for (eNB_id=0;eNB_id<NB_eNB_INST;eNB_id++) {
          if (min_path_loss<eNB2UE[eNB_id][UE_id][CC_id]->path_loss_dB) {
            min_path_loss = eNB2UE[eNB_id][UE_id][CC_id]->path_loss_dB;
            att_eNB_id=eNB_id;
            LOG_D(OCM,"B: UE attached to eNB (UE%d->eNB%d)\n",UE_id,eNB_id);
          }
        }
      }

      if (att_eNB_id<0) {
        LOG_E(OCM,"Cannot find eNB for UE %d, return\n",UE_id);
        return; //exit(-1);
      }
      
#ifdef DEBUG_SIM
      rx_pwr = signal_energy_fp2(eNB2UE[att_eNB_id][UE_id][CC_id]->ch[0],
                                 eNB2UE[att_eNB_id][UE_id][CC_id]->channel_length)*eNB2UE[att_eNB_id][UE_id][CC_id]->channel_length;
      LOG_D(OCM,"Channel (CCid %d) eNB %d => UE %d : tx_power %d dBm, path_loss %f dB\n",
            CC_id,att_eNB_id,UE_id,
            PHY_vars_eNB_g[att_eNB_id][CC_id]->lte_frame_parms.pdsch_config_common.referenceSignalPower,
            eNB2UE[att_eNB_id][UE_id][CC_id]->path_loss_dB);
#endif

      //dlsch_abstraction(PHY_vars_UE_g[UE_id]->sinr_dB, rb_alloc, 8);
      // fill in perfect channel estimates
      channel_desc_t *desc1 = eNB2UE[att_eNB_id][UE_id][CC_id];
      int32_t **dl_channel_est = PHY_vars_UE_g[UE_id][CC_id]->lte_ue_common_vars.dl_ch_estimates[0];
      //      double scale = pow(10.0,(enb_data[att_eNB_id]->tx_power_dBm + eNB2UE[att_eNB_id][UE_id]->path_loss_dB + (double) PHY_vars_UE_g[UE_id]->rx_total_gain_dB)/20.0);
      double scale = pow(10.0,(PHY_vars_eNB_g[att_eNB_id][CC_id]->lte_frame_parms.pdsch_config_common.referenceSignalPower+eNB2UE[att_eNB_id][UE_id][CC_id]->path_loss_dB + (double) PHY_vars_UE_g[UE_id][CC_id]->rx_total_gain_dB)/20.0);
      //this factor is not really needed (it was actually wrong in the non abstraction mode)
      //scale = scale * sqrt(512.0/300.0); //TODO: make this variable for all BWs
      LOG_D(OCM,"scale =%lf (%d dB)\n",scale,(int) (20*log10(scale)));
      // freq_channel(desc1,frame_parms->N_RB_DL,nb_samples);
      //write_output("channel.m","ch",desc1->ch[0],desc1->channel_length,1,8);
      //write_output("channelF.m","chF",desc1->chF[0],nb_samples,1,8);
      int count,count1,a_rx,a_tx;
      for(a_tx=0;a_tx<nb_antennas_tx;a_tx++)
	{ 
	  for (a_rx=0;a_rx<nb_antennas_rx;a_rx++)
	    {
	      //for (count=0;count<frame_parms->symbols_per_tti/2;count++)
	      for (count=0;count<1;count++)
		{ 
		  for (count1=0;count1<frame_parms->N_RB_DL*12;count1++)
		    { 
		      ((int16_t *) dl_channel_est[(a_tx<<1)+a_rx])[2*count1+(count*frame_parms->ofdm_symbol_size+LTE_CE_FILTER_LENGTH)*2]=(int16_t)(desc1->chF[a_rx+(a_tx*nb_antennas_rx)][count1].x*scale);
		      ((int16_t *) dl_channel_est[(a_tx<<1)+a_rx])[2*count1+1+(count*frame_parms->ofdm_symbol_size+LTE_CE_FILTER_LENGTH)*2]=(int16_t)(desc1->chF[a_rx+(a_tx*nb_antennas_rx)][count1].y*scale) ;
		    }
		}
	    }
	}

      /*
      if(PHY_vars_UE_g[UE_id]->transmission_mode[att_eNB_id]>=5)
	{
	  lte_ue_measurements(PHY_vars_UE_g[UE_id],
			      ((next_slot-1)>>1)*frame_parms->samples_per_tti,
			      1,
			      abstraction_flag);
	  		      
	  PHY_vars_eNB_g[att_eNB_id]->dlsch_eNB[0][0]->pmi_alloc = quantize_subband_pmi(&PHY_vars_UE_g[UE_id]->PHY_measurements,0);
	  //  printf("pmi_alloc in channel sim: %d",PHY_vars_eNB_g[att_eNB_id]->dlsch_eNB[0][0]->pmi_alloc);
        }
      */		      
      
      // calculate the SNR for the attached eNB (this assumes eNB always uses PMI stored in eNB_UE_stats; to be improved)
      init_snr(eNB2UE[att_eNB_id][UE_id][CC_id], enb_data[att_eNB_id], ue_data[UE_id], PHY_vars_UE_g[UE_id][CC_id]->sinr_dB, &PHY_vars_UE_g[UE_id][CC_id]->N0, PHY_vars_UE_g[UE_id][CC_id]->transmission_mode[att_eNB_id], PHY_vars_eNB_g[att_eNB_id][CC_id]->eNB_UE_stats[UE_id].DL_pmi_single,PHY_vars_eNB_g[att_eNB_id][CC_id]->mu_mimo_mode[UE_id].dl_pow_off);

      // calculate sinr here
      for (eNB_id = 0; eNB_id < NB_eNB_INST; eNB_id++) {
        if (att_eNB_id != eNB_id) {
          calculate_sinr(eNB2UE[eNB_id][UE_id][CC_id], enb_data[eNB_id], ue_data[UE_id], PHY_vars_UE_g[UE_id][CC_id]->sinr_dB);
        }
      }
    } // hold channel
  }
  
  else { //abstraction_flag
    /* 
       Call do_OFDM_mod from phy_procedures_eNB_TX function
    */
   
    for (eNB_id=0;eNB_id<NB_eNB_INST;eNB_id++) {
      start_meas(&PHY_vars_eNB_g[eNB_id][CC_id]->ofdm_mod_stats);
      do_OFDM_mod(PHY_vars_eNB_g[eNB_id][CC_id]->lte_eNB_common_vars.txdataF[0],
		  PHY_vars_eNB_g[eNB_id][CC_id]->lte_eNB_common_vars.txdata[0],
		  PHY_vars_eNB_g[eNB_id][CC_id]->proc[subframe_sched].frame_tx,next_slot,
		  &PHY_vars_eNB_g[eNB_id][CC_id]->lte_frame_parms);
      stop_meas(&PHY_vars_eNB_g[eNB_id][CC_id]->ofdm_mod_stats);
    }
   
    //for (UE_id=0;UE_id<NB_UE_INST;UE_id++) {
      // Compute RX signal for UE = UE_id
      /*
      for (i=0;i<(frame_parms->samples_per_tti>>1);i++) {
	for (aa=0;aa<nb_antennas_rx;aa++) {
	  r_re[aa][i]=0.0;
	  r_im[aa][i]=0.0;
	}
      }
      */
      //      printf("r_re[0] %p\n",r_re[0]);
      for (aa=0;aa<nb_antennas_rx;aa++) {
        memset((void*)r_re[aa],0,(frame_parms->samples_per_tti>>1)*sizeof(double));
        memset((void*)r_im[aa],0,(frame_parms->samples_per_tti>>1)*sizeof(double));
      }
      /*
      for (i=0;i<16;i++)
	printf("%f, %X\n",r_re[aa][i],(unsigned long long)r_re[aa][i]);
      */
      for (eNB_id=0;eNB_id<NB_eNB_INST;eNB_id++) {
        //	if (((double)PHY_vars_UE_g[UE_id]->tx_power_dBm +
        //	     eNB2UE[eNB_id][UE_id]->path_loss_dB) <= -107.0)
        //	  break;
        frame_parms = &PHY_vars_eNB_g[eNB_id][CC_id]->lte_frame_parms;
        txdata = PHY_vars_eNB_g[eNB_id][CC_id]->lte_eNB_common_vars.txdata[0];
        slot_offset = (next_slot)*(frame_parms->samples_per_tti>>1);
        slot_offset_meas = ((next_slot&1)==0) ? slot_offset : (slot_offset-(frame_parms->samples_per_tti>>1));
        tx_pwr = dac_fixed_gain(s_re,
                                s_im,
                                txdata,
                                slot_offset,
                                nb_antennas_tx,
                                frame_parms->samples_per_tti>>1,
                                slot_offset_meas,
                                frame_parms->ofdm_symbol_size,
                                14,
                                //				enb_data[eNB_id]->tx_power_dBm);
                                PHY_vars_eNB_g[eNB_id][CC_id]->lte_frame_parms.pdsch_config_common.referenceSignalPower);

#ifdef DEBUG_SIM
        LOG_D(OCM,"[SIM][DL] eNB %d (CCid %d): tx_pwr %f dBm, for slot %d (subframe %d)\n",
              eNB_id,CC_id,
              10*log10(tx_pwr),
              next_slot,
              next_slot>>1);
#endif
        //eNB2UE[eNB_id][UE_id]->path_loss_dB = 0;
        multipath_channel(eNB2UE[eNB_id][UE_id][CC_id],s_re,s_im,r_re0,r_im0,
                          frame_parms->samples_per_tti>>1,hold_channel);
#ifdef DEBUG_SIM	  
        rx_pwr = signal_energy_fp2(eNB2UE[eNB_id][UE_id][CC_id]->ch[0],
                                   eNB2UE[eNB_id][UE_id][CC_id]->channel_length)*eNB2UE[eNB_id][UE_id][CC_id]->channel_length;
        LOG_D(OCM,"[SIM][DL] Channel eNB %d => UE %d (CCid %d): Channel gain %f dB (%f)\n",eNB_id,UE_id,CC_id,10*log10(rx_pwr),rx_pwr);
#endif


#ifdef DEBUG_SIM
        for (i=0;i<eNB2UE[eNB_id][UE_id][CC_id]->channel_length;i++)
          printf("ch(%d,%d)[%d] : (%f,%f)\n",eNB_id,UE_id,i,eNB2UE[eNB_id][UE_id][CC_id]->ch[0][i].x,eNB2UE[eNB_id][UE_id][CC_id]->ch[0][i].y);
#endif

        LOG_D(OCM,"[SIM][DL] Channel eNB %d => UE %d (CCid %d): tx_power %f dBm, path_loss %f dB\n",
              eNB_id,UE_id,CC_id,
              (double)PHY_vars_eNB_g[eNB_id][CC_id]->lte_frame_parms.pdsch_config_common.referenceSignalPower,
              //	       enb_data[eNB_id]->tx_power_dBm,
              eNB2UE[eNB_id][UE_id][CC_id]->path_loss_dB);

#ifdef DEBUG_SIM      
        rx_pwr = signal_energy_fp(r_re0,r_im0,nb_antennas_rx,512,0)*512.0/300.0;
        LOG_D(OCM,"[SIM][DL] UE %d : rx_pwr %f dBm for slot %d (subframe %d)\n",UE_id,10*log10(rx_pwr),next_slot,next_slot>>1);
#endif      

        if (eNB2UE[eNB_id][UE_id][CC_id]->first_run == 1)
          eNB2UE[eNB_id][UE_id][CC_id]->first_run = 0;


        // RF model
#ifdef DEBUG_SIM
        LOG_D(OCM,"[SIM][DL] UE %d (CCid %d): rx_gain %d dB for slot %d (subframe %d)\n",UE_id,CC_id,PHY_vars_UE_g[UE_id][CC_id]->rx_total_gain_dB,next_slot,next_slot>>1);
#endif
        /*
	rf_rx(r_re0,
	      r_im0,
	      NULL,
	      NULL,
	      0,
	      nb_antennas_rx,
	      frame_parms->samples_per_tti>>1,
	      1e3/eNB2UE[eNB_id][UE_id]->BW,  // sampling time (ns)
	      0.0,               // freq offset (Hz) (-20kHz..20kHz)
	      0.0,               // drift (Hz) NOT YET IMPLEMENTED
	      ue_data[UE_id]->rx_noise_level,                // noise_figure NOT YET IMPLEMENTED
	      (double)PHY_vars_UE_g[UE_id]->rx_total_gain_dB - 66.227,   // rx_gain (dB) (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)
	      200.0,               // IP3_dBm (dBm)
	      &eNB2UE[eNB_id][UE_id]->ip,               // initial phase
	      30.0e3,            // pn_cutoff (kHz)
	      -500.0,            // pn_amp (dBc) default: 50
	      0.0,               // IQ imbalance (dB),
	      0.0);              // IQ phase imbalance (rad)
	*/

        rf_rx_simple(r_re0,
                     r_im0,
                     nb_antennas_rx,
                     frame_parms->samples_per_tti>>1,
                     1e3/eNB2UE[eNB_id][UE_id][CC_id]->BW,  // sampling time (ns)
                     (double)PHY_vars_UE_g[UE_id][CC_id]->rx_total_gain_dB - 66.227);   // rx_gain (dB) (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)

#ifdef DEBUG_SIM    
        rx_pwr = signal_energy_fp(r_re0,r_im0,nb_antennas_rx,frame_parms->ofdm_symbol_size,0)*512.0/300;
        LOG_D(OCM,"[SIM][DL] UE %d : ADC in (eNB %d) %f dB for slot %d (subframe %d)\n",
               UE_id,eNB_id,
               10*log10(rx_pwr),next_slot,next_slot>>1);
#endif    	
        for (i=0;i<(frame_parms->samples_per_tti>>1);i++) {
          for (aa=0;aa<nb_antennas_rx;aa++) {
            r_re[aa][i]+=r_re0[aa][i];
            r_im[aa][i]+=r_im0[aa][i];
          }
        }

      }      
#ifdef DEBUG_SIM    
      rx_pwr = signal_energy_fp(r_re,r_im,nb_antennas_rx,frame_parms->ofdm_symbol_size,0)*512.0/300.0;
      LOG_D(OCM,"[SIM][DL] UE %d : ADC in %f dB for slot %d (subframe %d)\n",UE_id,10*log10(rx_pwr),next_slot,next_slot>>1);  
#endif    

      rxdata = PHY_vars_UE_g[UE_id][CC_id]->lte_ue_common_vars.rxdata;
      slot_offset = (next_slot)*(frame_parms->samples_per_tti>>1);
      
      adc(r_re,
	  r_im,
	  0,
	  slot_offset,
	  rxdata,
	  nb_antennas_rx,
	  frame_parms->samples_per_tti>>1,
	  12);
      
#ifdef DEBUG_SIM
      rx_pwr2 = signal_energy(rxdata[0]+slot_offset,512)*512.0/300.0;
      LOG_D(OCM,"[SIM][DL] UE %d : rx_pwr (ADC out) %f dB (%d) for slot %d (subframe %d), writing to %p\n",UE_id, 10*log10((double)rx_pwr2),rx_pwr2,next_slot,next_slot>>1,rxdata);  
#else
      UNUSED_VARIABLE(rx_pwr2);
      UNUSED_VARIABLE(tx_pwr);
      UNUSED_VARIABLE(rx_pwr);
#endif
    //}// UE_index loop
  }

}


void do_UL_sig(double **r_re0,double **r_im0,double **r_re,double **r_im,double **s_re,double **s_im,channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX][MAX_NUM_CCs],node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],uint16_t next_slot,uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms, uint32_t frame,uint8_t CC_id) {

  int32_t **txdata,**rxdata;
#ifdef PHY_ABSTRACTION_UL
  int32_t att_eNB_id=-1;
#endif
  uint8_t eNB_id=0,UE_id=0;

  uint8_t nb_antennas_rx = UE2eNB[0][0][CC_id]->nb_rx; // number of rx antennas at eNB
  uint8_t nb_antennas_tx = UE2eNB[0][0][CC_id]->nb_tx; // number of tx antennas at UE

  double tx_pwr, rx_pwr;
  int32_t rx_pwr2;
  uint32_t i,aa;
  uint32_t slot_offset,slot_offset_meas;

  uint8_t hold_channel=0;

#ifdef PHY_ABSTRACTION_UL
  double min_path_loss=-200;
  uint16_t ul_nb_rb=0 ;
  uint16_t ul_fr_rb=0;
  int ulnbrb2 ;
  int ulfrrb2 ;
  uint8_t harq_pid;
  int subframe = (next_slot>>1);
#endif  

  /*
  if (next_slot==4) 
    hold_channel = 0;
  else
    hold_channel = 1;
  */

  if (abstraction_flag!=0)  {
#ifdef PHY_ABSTRACTION_UL
   for (eNB_id=0;eNB_id<NB_eNB_INST;eNB_id++) 
    {
      for (UE_id=0;UE_id<NB_UE_INST;UE_id++) 
      {
	if (!hold_channel) {
	  random_channel(UE2eNB[UE_id][eNB_id][CC_id],abstraction_flag);
	  freq_channel(UE2eNB[UE_id][eNB_id][CC_id], frame_parms->N_RB_UL,frame_parms->N_RB_UL*12+1);
	  
	  // REceived power at the eNB
	  rx_pwr = signal_energy_fp2(UE2eNB[UE_id][eNB_id][CC_id]->ch[0],
				     UE2eNB[UE_id][eNB_id][CC_id]->channel_length)*UE2eNB[UE_id][att_eNB_id][CC_id]->channel_length; // calculate the rx power at the eNB
	}

	//  write_output("SINRch.m","SINRch",PHY_vars_eNB_g[att_eNB_id]->sinr_dB_eNB,frame_parms->N_RB_UL*12+1,1,1);
	if(subframe>1 && subframe <5)
          {
            harq_pid = subframe2harq_pid(frame_parms,frame,subframe);
            ul_nb_rb = PHY_vars_eNB_g[att_eNB_id][CC_id]->ulsch_eNB[(uint8_t)UE_id]->harq_processes[harq_pid]->nb_rb;
            ul_fr_rb = PHY_vars_eNB_g[att_eNB_id][CC_id]->ulsch_eNB[(uint8_t)UE_id]->harq_processes[harq_pid]->first_rb;
          }
	
	if(ul_nb_rb>1 && (ul_fr_rb < 25 && ul_fr_rb > -1))
          {
            number_rb_ul = ul_nb_rb;
            first_rbUL = ul_fr_rb;
            init_snr_up(UE2eNB[UE_id][att_eNB_id][CC_id],enb_data[att_eNB_id], ue_data[UE_id],PHY_vars_eNB_g[att_eNB_id][CC_id]->sinr_dB,&PHY_vars_UE_g[att_eNB_id][CC_id]->N0,ul_nb_rb,ul_fr_rb);
	    
          }
      } //UE_id
    } //eNB_id
#else
/* the following functions are not needed */
/*  
if (abstraction_flag!=0) {
    for (eNB_id=0;eNB_id<NB_eNB_INST;eNB_id++) {
      for (UE_id=0;UE_id<NB_UE_INST;UE_id++) {
	random_channel(UE2eNB[UE_id][eNB_id]);
	freq_channel(UE2eNB[UE_id][eNB_id], frame_parms->N_RB_UL,2);
      }
    }
  }
*/
#endif
  }
  else { //without abstraction

    /*
    for (UE_id=0;UE_id<NB_UE_INST;UE_id++) {
      do_OFDM_mod(PHY_vars_UE_g[UE_id]->lte_ue_common_vars.txdataF,PHY_vars_UE_g[UE_id]->lte_ue_common_vars.txdata,next_slot,&PHY_vars_UE_g[UE_id]->lte_frame_parms);
    }
    */

    for (eNB_id=0;eNB_id<NB_eNB_INST;eNB_id++) {
      // Clear RX signal for eNB = eNB_id
      for (i=0;i<(frame_parms->samples_per_tti>>1);i++) {
        for (aa=0;aa<nb_antennas_rx;aa++) {
          r_re[aa][i]=0.0;
          r_im[aa][i]=0.0;
        }
      }
      
      // Compute RX signal for eNB = eNB_id
      for (UE_id=0;UE_id<NB_UE_INST;UE_id++){

        txdata = PHY_vars_UE_g[UE_id][CC_id]->lte_ue_common_vars.txdata;
        frame_parms = &PHY_vars_UE_g[UE_id][CC_id]->lte_frame_parms;
        slot_offset = (next_slot)*(frame_parms->samples_per_tti>>1);
        slot_offset_meas = ((next_slot&1)==0) ? slot_offset : (slot_offset-(frame_parms->samples_per_tti>>1));

        if (((double)PHY_vars_UE_g[UE_id][CC_id]->tx_power_dBm +
             UE2eNB[UE_id][eNB_id][CC_id]->path_loss_dB) <= -125.0) {

          // don't simulate a UE that is too weak
        }
        else {

          tx_pwr = dac_fixed_gain(s_re,
                                  s_im,
                                  txdata,
                                  slot_offset,
                                  nb_antennas_tx,
                                  frame_parms->samples_per_tti>>1,
                                  slot_offset_meas,
                                  frame_parms->ofdm_symbol_size,
                                  14,
                                  PHY_vars_UE_g[UE_id][CC_id]->tx_power_dBm);
          //ue_data[UE_id]->tx_power_dBm);
#ifdef DEBUG_SIM
	  LOG_D(OCM,"[SIM][UL] UE %d tx_pwr %f dBm (target %d dBm) for slot %d (subframe %d, slot_offset %d, slot_offset_meas %d)\n",UE_id,10*log10(tx_pwr),PHY_vars_UE_g[UE_id]->tx_power_dBm,next_slot,next_slot>>1,slot_offset,slot_offset_meas);
#endif
	  
	  multipath_channel(UE2eNB[UE_id][eNB_id][CC_id],s_re,s_im,r_re0,r_im0,
			    frame_parms->samples_per_tti>>1,hold_channel);

#ifdef DEBUG_SIM	  
          rx_pwr = signal_energy_fp2(UE2eNB[UE_id][eNB_id][CC_id]->ch[0],
                                     UE2eNB[UE_id][eNB_id][CC_id]->channel_length)*UE2eNB[UE_id][eNB_id][CC_id]->channel_length;
          LOG_D(OCM,"[SIM][UL] slot %d Channel UE %d => eNB %d : %f dB (hold %d)\n",next_slot,UE_id,eNB_id,10*log10(rx_pwr),hold_channel);
#endif

#ifdef DEBUG_SIM    
	  rx_pwr = signal_energy_fp(r_re0,r_im0,nb_antennas_rx,frame_parms->samples_per_tti>>1,0);
	  LOG_D(OCM,"[SIM][UL] eNB %d : rx_pwr %f dB (%f) for slot %d (subframe %d), sptti %d\n",
		 eNB_id,10*log10(rx_pwr),rx_pwr,next_slot,next_slot>>1,frame_parms->samples_per_tti);  
#endif


          if (UE2eNB[UE_id][eNB_id][CC_id]->first_run == 1)
            UE2eNB[UE_id][eNB_id][CC_id]->first_run = 0;



          for (aa=0;aa<nb_antennas_rx;aa++) {
            for (i=0;i<(frame_parms->samples_per_tti>>1);i++) {
              r_re[aa][i]+=r_re0[aa][i];
              r_im[aa][i]+=r_im0[aa][i];
            }
          }
        }
      } //UE_id
      
      // RF model
      /*
	  rf_rx(r_re0,
	  r_im0,
	  NULL,
	  NULL,
	  0,
	  frame_parms->nb_antennas_rx,
	  frame_parms->samples_per_tti>>1,
	  1e3/UE2eNB[UE_id][eNB_id]->BW,  // sampling time (ns) 
	  0.0,               // freq offset (Hz) (-20kHz..20kHz)
	  0.0,               // drift (Hz) NOT YET IMPLEMENTED
	  enb_data[eNB_id]->rx_noise_level,                // noise_figure NOT YET IMPLEMENTED
	  (double)PHY_vars_eNB_g[eNB_id]->rx_total_gain_eNB_dB - 66.227,   // rx_gain (dB) (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)
	  200.0,               // IP3_dBm (dBm)
	  &UE2eNB[UE_id][eNB_id]->ip,               // initial phase
	  30.0e3,            // pn_cutoff (kHz)
	  -500.0,            // pn_amp (dBc) default: 50
	  0.0,               // IQ imbalance (dB),
	  0.0);              // IQ phase imbalance (rad)
	*/
      
      rf_rx_simple(r_re,
                   r_im,
                   nb_antennas_rx,
                   frame_parms->samples_per_tti>>1,
                   1e3/UE2eNB[0][eNB_id][CC_id]->BW,  // sampling time (ns)
                   (double)PHY_vars_eNB_g[eNB_id][CC_id]->rx_total_gain_eNB_dB - 66.227);   // rx_gain (dB) (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)

#ifdef DEBUG_SIM    
      rx_pwr = signal_energy_fp(r_re,r_im,nb_antennas_rx,frame_parms->samples_per_tti>>1,0);
      LOG_D(OCM,"[SIM][UL] rx_pwr (ADC in) %f dB for slot %d (subframe %d)\n",10*log10(rx_pwr),next_slot,next_slot>>1);  
#endif
      
      rxdata = PHY_vars_eNB_g[eNB_id][CC_id]->lte_eNB_common_vars.rxdata[0];
      slot_offset = (next_slot)*(frame_parms->samples_per_tti>>1);
      
      adc(r_re,
          r_im,
          0,
          slot_offset,
          rxdata,
          nb_antennas_rx,
          frame_parms->samples_per_tti>>1,
          12);
      
#ifdef DEBUG_SIM    
      rx_pwr2 = signal_energy(rxdata[0]+slot_offset,frame_parms->samples_per_tti>>1);
      LOG_D(OCM,"[SIM][UL] eNB %d rx_pwr (ADC out) %f dB (%d) for slot %d (subframe %d)\n",eNB_id,10*log10((double)rx_pwr2),rx_pwr2,next_slot,next_slot>>1);  
#else
      UNUSED_VARIABLE(tx_pwr);
      UNUSED_VARIABLE(rx_pwr);
      UNUSED_VARIABLE(rx_pwr2);
#endif
      
    } // eNB_id
  } // abstraction_flag==0

}


void init_channel_vars(LTE_DL_FRAME_PARMS *frame_parms, double ***s_re,double ***s_im,double ***r_re,double ***r_im,double ***r_re0,double ***r_im0) {

  int i;

  *s_re = malloc(2*sizeof(double*));
  *s_im = malloc(2*sizeof(double*));
  *r_re = malloc(2*sizeof(double*));
  *r_im = malloc(2*sizeof(double*));
  *r_re0 = malloc(2*sizeof(double*));
  *r_im0 = malloc(2*sizeof(double*));


  for (i=0;i<2;i++) {

    (*s_re)[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero((*s_re)[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    (*s_im)[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero((*s_im)[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    (*r_re)[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero((*r_re)[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    (*r_im)[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero((*r_im)[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    (*r_re0)[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero((*r_re0)[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    (*r_im0)[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero((*r_im0)[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
  }
}


