#include "monique_core_Datastructures.h"
#include "monique_core_Parameters.h"
#include "monique_core_Synth.h"

//==============================================================================
//==============================================================================
//==============================================================================
static inline float positive( float x ) noexcept
{
    return x < 0 ? x * -1 : x;
}

//==============================================================================
//==============================================================================
//==============================================================================
static inline int reduce_id_to_smaller_100( int id_ ) noexcept
{
    if( id_ >= 500 )
    {
        id_ -= 500;
    }
    else if( id_ >= 400 )
    {
        id_ -= 400;
    }
    else if( id_ >= 300 )
    {
        id_ -= 300;
    }
    else if( id_ >= 200 )
    {
        id_ -= 200;
    }
    else if( id_ >= 100 )
    {
        id_ -= 100;
    }

    return id_;
}

//==============================================================================
//==============================================================================
//==============================================================================
COLD RuntimeListener::RuntimeListener( RuntimeNotifyer*const notifyer_ ) noexcept :
notifyer( notifyer_ ),
          sample_rate( notifyer_ ? notifyer_->sample_rate : 1 ),
          sample_rate_1ths( notifyer_ ? notifyer_->sample_rate_1ths : 1 ),
          block_size( notifyer_ ? notifyer_->block_size : 1 )
{
    if( notifyer )
    {
        notifyer->listeners.add( this );
        notifyer->listeners.minimiseStorageOverheads();
    }
}

COLD RuntimeListener::~RuntimeListener() noexcept
{
    if( notifyer )
    {
        notifyer->listeners.removeFirstMatchingValue( this );
    }
}

//==============================================================================
COLD void RuntimeListener::set_sample_rate( double sr_ ) noexcept
{
    sample_rate = sr_;
    sample_rate_1ths = 1.0f/sample_rate;
};
COLD void RuntimeListener::set_block_size( int bs_ ) noexcept { block_size = bs_; };
COLD void RuntimeListener::sample_rate_changed( double /* old_sr_ */ ) noexcept {};
COLD void RuntimeListener::block_size_changed() noexcept {};

//==============================================================================
//==============================================================================
//==============================================================================
COLD RuntimeNotifyer::RuntimeNotifyer() noexcept :
sample_rate(44100),
            sample_rate_1ths( 1.0/44100),
block_size(512) {}
COLD RuntimeNotifyer::~RuntimeNotifyer() noexcept {}

//==============================================================================
COLD void RuntimeNotifyer::set_sample_rate( double sr_ ) noexcept
{
    double old_sr = sample_rate;
    sample_rate = sr_;
    sample_rate_1ths = 1.0/sample_rate;
    for( int i = 0 ; i != listeners.size() ; ++i )
    {
        listeners[i]->set_sample_rate(sr_);
        listeners[i]->sample_rate_changed(old_sr);
    }
};
COLD void RuntimeNotifyer::set_block_size( int bs_ ) noexcept
{
    block_size = bs_;
    for( int i = 0 ; i != listeners.size() ; ++i ) {
        listeners[i]->set_block_size(bs_);
        listeners[i]->block_size_changed();
    }
};


//==============================================================================
//==============================================================================
//==============================================================================
COLD RuntimeInfo::RuntimeInfo() noexcept
:
samples_since_start(0),
                    bpm(120)
#ifdef IS_STANDALONE
                    ,
                    is_extern_synced(false),
                    clock_counter()
#endif
{
    std::cout << "MONIQUE: init RTI" << std::endl;
}
COLD RuntimeInfo::~RuntimeInfo() noexcept {}

//==============================================================================
//==============================================================================
//==============================================================================
#define LFO_NAME "LFO"
COLD LFOData::LFOData( SmoothManager*smooth_manager_, int id_ ) noexcept
:
speed
(
    MIN_MAX( 0, 16+127-33 ),
    4,
    (16+127-33)*10,

    generate_param_name(LFO_NAME,id_,"speed"),
    generate_short_human_name(LFO_NAME,id_,"speed")
)
{}
COLD LFOData::~LFOData() noexcept {}

//==============================================================================
static inline void copy( LFOData* dest_, const LFOData* src_ ) noexcept
{
    dest_->speed = src_->speed;
}
static inline void collect_saveable_parameters( LFOData* lfo_data_, Array< Parameter* >& params_ ) noexcept
{
    params_.add( &lfo_data_->speed );
}

//==============================================================================
//==============================================================================
//==============================================================================
#define LFO_COMPLEX_NAME "MFO"
COLD MFOData::MFOData( SmoothManager*const smooth_manager_, int id_ ) noexcept
:
speed
(
    MIN_MAX( 0, 16 ),
    4,
    16,

    generate_param_name(LFO_COMPLEX_NAME,id_,"speed"),
    generate_short_human_name(LFO_COMPLEX_NAME,id_,"speed")
),
wave
(
    MIN_MAX( 0, 1 ),
    0,
    1000,
    generate_param_name(LFO_COMPLEX_NAME,id_,"wave"),
    generate_short_human_name(LFO_COMPLEX_NAME,id_,"wave")
),
wave_smoother( smooth_manager_, &wave ),
phase_shift
(
    MIN_MAX( 0, 1 ),
    0,
    1000,
    generate_param_name(LFO_COMPLEX_NAME,id_,"phase"),
    generate_short_human_name(LFO_COMPLEX_NAME,id_,"phase")
),
phase_shift_smoother(smooth_manager_,&phase_shift)
{}
COLD MFOData::~MFOData() noexcept {}

//==============================================================================
static inline void collect_saveable_parameters( MFOData* lfo_data_, Array< Parameter* >& params_ ) noexcept
{
    params_.add( &lfo_data_->speed );
    params_.add( &lfo_data_->wave );
    params_.add( &lfo_data_->phase_shift );
}

//==============================================================================
//==============================================================================
//==============================================================================

#define OSC_NAME "OSC"
#define FM_NAME "FM"
COLD FMOscData::FMOscData( SmoothManager*const smooth_manager_ ) noexcept
:
fm_freq
(
    MIN_MAX( 0, 1 ),
    0,
    1000,
    generate_param_name(OSC_NAME,MASTER_OSC,"fm_multi"),
    generate_short_human_name(FM_NAME,"tune")
),
fm_freq_smoother(smooth_manager_,&fm_freq),
sync
(
    true,
    generate_param_name(OSC_NAME,MASTER_OSC, "sync" ),
    generate_short_human_name
    (
        FM_NAME,
        "sync"
    )
),
fm_swing
(
    MIN_MAX( 0, 1 ),
    0,
    5000,
    generate_param_name(OSC_NAME,MASTER_OSC,"fm_swing"),
    generate_short_human_name(FM_NAME,"swing")
),
fm_swing_smoother(smooth_manager_,&fm_swing),
fm_shape
(
    MIN_MAX( 0, 1 ),
    0,
    1000,
    generate_param_name(OSC_NAME,MASTER_OSC,"fm_phase"),
    generate_short_human_name(FM_NAME,"phase")
),
fm_shape_smoother(smooth_manager_,&fm_shape),
master_shift
(
    MIN_MAX( 0, 1 ),
    0,
    1000,
    generate_param_name(OSC_NAME,MASTER_OSC,"master_shift"),
    generate_short_human_name(OSC_NAME,"phase"),
    0
),
master_shift_smoother(smooth_manager_,&master_shift)
{
}
COLD FMOscData::~FMOscData() noexcept {}

//==============================================================================
static inline void copy( FMOscData* dest_, const FMOscData* src_ ) noexcept
{
    dest_->fm_freq = src_->fm_freq;
    dest_->sync = src_->sync;
    dest_->fm_swing = src_->fm_swing;
    dest_->master_shift = src_->master_shift;
    dest_->fm_shape = src_->fm_shape;
}
static inline void collect_saveable_parameters( FMOscData* osc_data_, Array< Parameter* >& params_ ) noexcept
{
    params_.add( &osc_data_->fm_freq );
    params_.add( &osc_data_->sync );
    params_.add( &osc_data_->fm_swing );
    params_.add( &osc_data_->master_shift );
    params_.add( &osc_data_->fm_shape );
}

//==============================================================================
//==============================================================================
//==============================================================================
COLD OSCData::OSCData( SmoothManager*const smooth_manager_, int id_ ) noexcept
:
id(id_),
sync
(
    true,
    generate_param_name(OSC_NAME,id_, id_ == MASTER_OSC ? "spacer" : "sync" ),
    generate_short_human_name
    (
        OSC_NAME,
        id_,
        "sync"
    )
),
wave
(
    MIN_MAX( SINE, NOICE ),
    SINE,
    3000,
    generate_param_name(OSC_NAME,id_,"wave"),
    generate_short_human_name(OSC_NAME,id_,"wave")
),
wave_smoother(smooth_manager_,&wave),
fm_amount
(
    MIN_MAX( 0, 1 ),
    0,
    1000,
    generate_param_name(OSC_NAME,id_,"fm_power"),
    generate_short_human_name(OSC_NAME,id_,"fm_mass")
),
fm_amount_smoother(smooth_manager_,&fm_amount),
tune
(
    MIN_MAX( -24, 24 ),
    0,
    1000*48,
    generate_param_name(OSC_NAME,id_,"octave"),
    generate_short_human_name(OSC_NAME,id_,"tune"),
    0.5 // one octave
),
tune_smoother(smooth_manager_,&tune),
is_lfo_modulated
(
    false,
    generate_param_name(OSC_NAME,id_,"is_lfo_mod"),
    generate_short_human_name(OSC_NAME,id_,"l-mod_ON")
),
last_modulation_value( 0 )
{
    if( id_ == MASTER_OSC )
    {
        tune_smoother.set_offline();
    }
}
COLD OSCData::~OSCData() noexcept {}

//==============================================================================
static inline void copy( OSCData* dest_, const OSCData* src_ ) noexcept
{
    dest_->wave = src_->wave;
    dest_->fm_amount = src_->fm_amount;
    dest_->is_lfo_modulated = src_->is_lfo_modulated;
    if( dest_->id != MASTER_OSC )
        dest_->tune = src_->tune;
    dest_->sync = src_->sync;
}
static inline void collect_saveable_parameters( OSCData* osc_data_, Array< Parameter* >& params_ ) noexcept
{
    params_.add( &osc_data_->sync );
    params_.add( &osc_data_->wave );
    params_.add( &osc_data_->fm_amount );
    if( osc_data_->id != MASTER_OSC )
        params_.add( &osc_data_->tune );
    params_.add( &osc_data_->is_lfo_modulated );
}

//==============================================================================
//==============================================================================
//==============================================================================
#define ENV_NAME "ENV"
COLD ENVData::ENVData( SmoothManager*const smooth_manager_, int id_ ) noexcept
:
id( id_ ),

attack
(
    MIN_MAX( 0, 1 ),
    0.05,
    5000,
    generate_param_name(ENV_NAME,id,"attack"),
    generate_short_human_name(ENV_NAME,id_,"attack")
),
decay
(
    MIN_MAX( 0, 1 ),
    0.02,
    5000,
    generate_param_name(ENV_NAME,id,"decay"),
    generate_short_human_name(ENV_NAME,id_,"decay")
),
sustain
(
    MIN_MAX( 0, 1 ),
    0.9,
    1000,
    generate_param_name(ENV_NAME,id,"sustain"),
    generate_short_human_name(ENV_NAME,id_,"sustain")
),
sustain_smoother(smooth_manager_,&sustain),
sustain_time
(
    MIN_MAX( 0, 1 ),
    1,
    MAX_ENV_TIMES,
    generate_param_name(ENV_NAME,id,"sustain_time"),
    generate_short_human_name(ENV_NAME,id_,"sus_time")),

release
(
    MIN_MAX( 0, 1 ),
    0.2,
    5000,
    generate_param_name(ENV_NAME,id,"release"),
    generate_short_human_name(ENV_NAME,id_,"release")
),
shape
(
    MIN_MAX( 0, 1 ),
    0.5,
    1000,
    generate_param_name(ENV_NAME,id,"shape"),
    generate_short_human_name(ENV_NAME,id_,"shape")
),
shape_smoother( smooth_manager_ , &shape )
{
}
COLD ENVData::~ENVData() noexcept {}

//==============================================================================
static inline void collect_saveable_parameters( ENVData* data_, Array< Parameter* >& params_ ) noexcept
{
    params_.add( &data_->attack );
    params_.add( &data_->decay );
    params_.add( &data_->sustain );
    params_.add( &data_->sustain_time );
    params_.add( &data_->release );

    params_.add( &data_->shape );
}

//==============================================================================
//==============================================================================
//==============================================================================
#define FILTER_NAME "FLT"
#define FILTER_NAME_SHORT "F"
COLD FilterData::FilterData( SmoothManager*const smooth_manager_, int id_ ) noexcept
:
// ----
filter_type
(
    MIN_MAX( LPF_2_PASS, MOOG_AND_LPF ),
    LPF_2_PASS,
    generate_param_name(FILTER_NAME,id_,"filter_type"),
    generate_short_human_name(FILTER_NAME_SHORT,id_,"type")
),

// ----
adsr_lfo_mix
(
    MIN_MAX( -1, 1 ),
    -0.9,
    2000,
    generate_param_name(FILTER_NAME,id_,"adsr_lfo_mix"),
    generate_short_human_name(FILTER_NAME_SHORT,id_,"mod_mix")
),
adsr_lfo_mix_smoother(smooth_manager_,&adsr_lfo_mix),

// ----
distortion
(
    MIN_MAX( 0, 1 ),
    0,
    1000,
    generate_param_name(FILTER_NAME,id_,"distortion"),
    generate_short_human_name(FILTER_NAME_SHORT,id_,"destroy"),
    0.6
),
distortion_smoother(smooth_manager_,&distortion),
modulate_distortion
(
    false,
    generate_param_name(FILTER_NAME,id_,"modulate_distortion"),
    generate_short_human_name(FILTER_NAME_SHORT,id_,"mod_destroy_ON")
),

// ----
cutoff
(
    MIN_MAX( 0, 1 ),
    0.2,
    MAX_CUTOFF,
    generate_param_name(FILTER_NAME,id_,"cutoff"),
    generate_short_human_name(FILTER_NAME_SHORT,id_,"cutoff"),
    0.7
),
cutoff_smoother(smooth_manager_,&cutoff),
modulate_cutoff
(
    true,
    generate_param_name(FILTER_NAME,id_,"modulate_cutoff"),
    generate_short_human_name(FILTER_NAME_SHORT,id_,"mod_cutoff_ON")
),

// ----
resonance
(
    MIN_MAX( 0, 1 ),
    0.3,
    1000,
    generate_param_name(FILTER_NAME,id_,"resonance"),
    generate_short_human_name(FILTER_NAME_SHORT,id_,"resonance"),
    0.2
),
resonance_smoother(smooth_manager_,&resonance),
modulate_resonance
(
    true,
    generate_param_name(FILTER_NAME,id_,"modulate_resonance"),
    generate_short_human_name(FILTER_NAME_SHORT,id_,"mod_resonance_ON")
),

// ----
pan
(
    MIN_MAX( -1, 1 ),
    0,
    2000,
    generate_param_name(FILTER_NAME,id_,"pan"),
    generate_short_human_name(FILTER_NAME_SHORT,id_,"pan"),
    0.0
),
pan_smoother(smooth_manager_,&pan),
modulate_pan
(
    false,
    generate_param_name(FILTER_NAME,id_,"modulate_pan"),
    generate_short_human_name(FILTER_NAME_SHORT,id_,"mod_pan_ON")
),

output
(
    MIN_MAX( 0, 1 ),
    0.75,
    1000,
    generate_param_name(FILTER_NAME,id_,"output"),
    generate_short_human_name(FILTER_NAME_SHORT,id_,"volume"),
    0.6
),
output_smoother(smooth_manager_,&output),

modulate_output
(
    false,
    generate_param_name(FILTER_NAME,id_,"modulate_output"),
    generate_short_human_name(FILTER_NAME_SHORT,id_,"mod_volume_ON")
),

// ----
input_sustains
(
    SUM_INPUTS_PER_FILTER,

    MIN_MAX( id_ == 0 ? 0 : -1, 1 ),
    0,
    id_ == 0 ? 1000 : 2000,

    FILTER_NAME,FILTER_NAME_SHORT,
    id_,
    "input_sustain","osc_input",true
),
input_holds
(
    SUM_INPUTS_PER_FILTER,

    true,

    FILTER_NAME,FILTER_NAME_SHORT,
    id_,
    "input_hold","env_ON",true
),

// ----
input_envs( /* INIT IN BODY */ ),
env_data( new ENVData( smooth_manager_, id_ ) )
{
    for( int i = 0 ; i != SUM_INPUTS_PER_FILTER ; ++i )
    {
        input_smoothers.add( new SmoothedParameter(smooth_manager_,&input_sustains[i]) );

        ENVData* env_data = new ENVData( smooth_manager_, i+id_*SUM_INPUTS_PER_FILTER+FILTER_INPUT_ENV_ID_OFFSET );
        input_envs.add( env_data );
    }
}

COLD FilterData::~FilterData() noexcept
{
    delete env_data;
}

//==============================================================================
static inline void copy( FilterData* dest_, const FilterData* src_ ) noexcept
{
    dest_->filter_type = src_->filter_type;
    dest_->adsr_lfo_mix = src_->adsr_lfo_mix;
    dest_->distortion = src_->distortion;
    dest_->modulate_distortion = src_->modulate_distortion;
    dest_->cutoff = src_->cutoff;
    dest_->modulate_cutoff = src_->modulate_cutoff;
    dest_->resonance = src_->resonance;
    dest_->modulate_resonance = src_->modulate_resonance;
    dest_->output = src_->output;
    dest_->modulate_output = src_->modulate_output;
    dest_->pan = src_->pan;
    dest_->modulate_pan = src_->modulate_pan;

    for( int i = 0 ; i != SUM_INPUTS_PER_FILTER ; ++i )
    {
        dest_->input_holds[i] = src_->input_holds[i];
        dest_->input_sustains[i] = src_->input_sustains[i];

        copy( dest_->input_envs.getUnchecked(i), src_->input_envs.getUnchecked(i) );
    }

    copy( dest_->env_data, src_->env_data );
}
static inline void collect_saveable_parameters( FilterData* data_, Array< Parameter* >& params_ ) noexcept
{
    for( int i = 0 ; i != SUM_INPUTS_PER_FILTER ; ++i )
    {
        params_.add( &data_->input_sustains[i] );
        params_.add( &data_->input_holds[i] );
        collect_saveable_parameters( data_->input_envs.getUnchecked(i), params_ );
    }
    collect_saveable_parameters( data_->env_data, params_ );

    params_.add( &data_->adsr_lfo_mix );

    params_.add( &data_->filter_type );

    params_.add( &data_->cutoff );
    params_.add( &data_->modulate_cutoff );
    params_.add( &data_->resonance );
    params_.add( &data_->modulate_resonance );

    params_.add( &data_->distortion );
    params_.add( &data_->modulate_distortion );

    params_.add( &data_->output );
    params_.add( &data_->modulate_output );

    params_.add( &data_->pan );
    params_.add( &data_->modulate_pan );
}

//==============================================================================
//==============================================================================
//==============================================================================
#define ARP_NAME "ARP"
COLD ArpSequencerData::ArpSequencerData( int id_ ) noexcept
:
is_on
(
    true,
    generate_param_name(ARP_NAME,id_,"is_on"),
    generate_short_human_name(ARP_NAME,"on")
),

// ----
step
(
    SUM_ENV_ARP_STEPS,

    false,

    ARP_NAME,ARP_NAME,
    id_,
    "step","step", false
),
tune
(
    SUM_ENV_ARP_STEPS,

    MIN_MAX( -48, 48 ),
    0,

    ARP_NAME,ARP_NAME,
    id_,
    "tune","tune", false
),
velocity
(
    SUM_ENV_ARP_STEPS,

    MIN_MAX( 0, 1 ),
    0.85,
    1000,

    ARP_NAME,ARP_NAME,
    id_,
    "velocity","velocity", false
),

// ----
shuffle
(
    MIN_MAX( 0, 15 ),
    0,
    generate_param_name(ARP_NAME,id_,"shuffle"),
    generate_short_human_name(ARP_NAME,"shuffle")
),
connect
(
    false,
    generate_param_name(ARP_NAME,id_,"connect"),
    generate_short_human_name(ARP_NAME,"connect")
),

// ----
speed_multi
(
    MIN_MAX( -15, 15 ),
    0,
    generate_param_name(ARP_NAME,id_,"speed_multi"),
    generate_short_human_name(ARP_NAME,"speed_multi")
)
{}

COLD ArpSequencerData::~ArpSequencerData() noexcept {}

//==============================================================================
static inline void copy( ArpSequencerData* dest_, const ArpSequencerData* src_ ) noexcept
{
    dest_->is_on = src_->is_on;

    for( int i = 0 ; i != SUM_ENV_ARP_STEPS ; ++i )
    {

        dest_->step[i] = src_->step[i];
        dest_->tune[i] = src_->tune[i];
        dest_->velocity[i] = src_->velocity[i];
    }

    dest_->shuffle = src_->shuffle;
    dest_->connect = src_->connect;
    dest_->speed_multi = src_->speed_multi;
}
static inline void collect_saveable_parameters( ArpSequencerData* data_, Array< Parameter* >& params_ ) noexcept
{
    params_.add( &data_->is_on );

    for( int i = 0 ; i != SUM_ENV_ARP_STEPS ; ++i )
    {
        params_.add( &data_->step[i] );
        params_.add( &data_->tune[i] );
        params_.add( &data_->velocity[i] );
    }

    params_.add( &data_->shuffle );
    params_.add( &data_->connect );

    params_.add( &data_->speed_multi );
}

//==============================================================================
//==============================================================================
//==============================================================================
#define EQ_NAME "EQ"
COLD EQData::EQData( SmoothManager*const smooth_manager_, int id_ ) noexcept
:
velocity
(
    SUM_EQ_BANDS,

    MIN_MAX( 0, 1 ),
    0.5,
    1000,

    EQ_NAME,EQ_NAME,
    id_,
    "velocity","band_power", false
),
hold
(
    SUM_EQ_BANDS,

    true,

    EQ_NAME,EQ_NAME,
    id_,
    "hold","env_ON", false
),
bypass
(
    MIN_MAX( 0, 1 ),
    1.0f,
    1000,
    generate_param_name(EQ_NAME,id_,"mix"),
    generate_short_human_name(EQ_NAME,"mix")
),
bypass_smoother(smooth_manager_,&bypass)
{
    for( int band_id = 0 ; band_id != SUM_EQ_BANDS ; ++band_id )
    {
        velocity_smoothers.add( new SmoothedParameter( smooth_manager_, &velocity[band_id] ) );

        ENVData* env_data = new ENVData( smooth_manager_, band_id+EQ_ENV_ID_OFFSET );
        envs.add( env_data );
    }
}
COLD EQData::~EQData() noexcept {}

//==============================================================================
static inline void copy( EQData* dest_, const EQData* src_ ) noexcept
{
    for( int i = 0 ; i != SUM_EQ_BANDS ; ++i )
    {
        dest_->velocity[i] = src_->velocity[i];
        dest_->hold[i] = src_->hold[i];
        copy( dest_->envs.getUnchecked( i ), src_->envs.getUnchecked( i ) );
    }

    dest_->bypass = src_->bypass;
}
static inline void collect_saveable_parameters( EQData* data_, Array< Parameter* >& params_ ) noexcept
{
    for( int i = 0 ; i != SUM_EQ_BANDS ; ++i )
    {
        params_.add( &data_->velocity[i] );
        params_.add( &data_->hold[i] );
        collect_saveable_parameters( data_->envs.getUnchecked( i ), params_ );
    }

    params_.add( &data_->bypass );
}

//==============================================================================
//==============================================================================
//==============================================================================
#define REVERB_NAME "VERB"
COLD ReverbData::ReverbData( SmoothManager*const smooth_manager_, int id_ ) noexcept
:
room
(
    MIN_MAX( 0, 1 ),
    0.333,
    1000,
    generate_param_name(REVERB_NAME,id_,"room"),
    generate_short_human_name("FX","r_room")
),
room_smoother( smooth_manager_, &room ),
dry_wet_mix
(
    MIN_MAX( 0, 1 ),
    0.75,
    1000,
    generate_param_name(REVERB_NAME,id_,"dry-wet"),
    generate_short_human_name("FX","r_dry-wet")
),
dry_wet_mix_smoother( smooth_manager_, &dry_wet_mix ),
width
(
    MIN_MAX( 0, 1 ),
    0.3,
    1000,
    generate_param_name(REVERB_NAME,id_,"width"),
    generate_short_human_name("FX","r_width")
),
width_smoother( smooth_manager_, &width ),
pan
(
    MIN_MAX( -1, 1 ),
    0,
    2000,
    generate_param_name(REVERB_NAME,id_,"pan"),
    generate_short_human_name("FX","reverb_pan"),
    0.0
),
pan_smoother(smooth_manager_,&pan)
{}

COLD ReverbData::~ReverbData() noexcept {}

//==============================================================================
static inline void copy( ReverbData* dest_, const ReverbData* src_ ) noexcept
{
    dest_->room = src_->room;
    dest_->width = src_->width;
    dest_->dry_wet_mix = src_->dry_wet_mix;
    dest_->pan = src_->pan;
}
static inline void collect_saveable_parameters( ReverbData* data_, Array< Parameter* >& params_ ) noexcept
{
    params_.add( &data_->room );
    params_.add( &data_->width );
    params_.add( &data_->dry_wet_mix );
    params_.add( &data_->pan );
}

//==============================================================================
//==============================================================================
//==============================================================================
#define CHORUS_NAME "CHR"
COLD ChorusData::ChorusData( SmoothManager*const smooth_manager_, int id_ ) noexcept
:
modulation
(
    MIN_MAX( 0, 1 ),
    0.333,
    1000,
    generate_param_name(CHORUS_NAME,id_,"modulation"),
    generate_short_human_name("FX","chorus-amount")
),
modulation_smoother( smooth_manager_, &modulation ),
pan
(
    MIN_MAX( -1, 1 ),
    0,
    2000,
    generate_param_name(CHORUS_NAME,id_,"pan"),
    generate_short_human_name("FX","chorus_pan"),
    0.0
),
pan_smoother(smooth_manager_,&pan)
{
}
COLD ChorusData::~ChorusData() noexcept
{
}

//==============================================================================
static inline void copy( ChorusData* dest_, const ChorusData* src_ ) noexcept
{
    dest_->modulation = src_->modulation;
    dest_->pan = src_->pan;
}
static inline void collect_saveable_parameters( ChorusData* data_, Array< Parameter* >& params_ ) noexcept
{
    params_.add( &data_->modulation );
    params_.add( &data_->pan );
}

//==============================================================================
COLD MorphGroup::MorphGroup() noexcept
:
left_morph_source( nullptr ),
right_morph_source( nullptr ),
last_power_of_right(0),
current_switch(LEFT),
current_callbacks(-1)
{}

COLD MorphGroup::~MorphGroup() noexcept {}

//==============================================================================
COLD void MorphGroup::register_parameter( Parameter* param_, bool is_master_ ) noexcept
{
    params.add( param_ );

    if( is_master_ )
    {
        param_->register_listener(this);
    }
}
COLD void MorphGroup::register_switch_parameter( BoolParameter* param_, bool is_master_ ) noexcept
{
    /*
    switch_bool_params.add( param_ );

    if( is_master_ )
      param_->register_listener(this);
    */
}
COLD void MorphGroup::register_switch_parameter( IntParameter* param_, bool is_master_ ) noexcept
{
    /*
    switch_int_params.add( param_ );

    if( is_master_ )
        param_->register_listener(this);
    */
}
COLD void MorphGroup::set_sources( MorphGroup* left_source_, MorphGroup* right_source_, float current_morph_amount_, bool current_switch_state_ ) noexcept
{
    last_power_of_right = current_morph_amount_;
    current_switch = current_switch_state_;

    left_morph_source = left_source_;
    right_morph_source = right_source_;
}

//==============================================================================
void MorphGroup::morph( float power_of_right_ ) noexcept
{
    for( int i = 0 ; i != params.size() ; ++i )
    {
        Parameter*target_param = params.getUnchecked(i);
        const Parameter*left_param = left_morph_source->params.getUnchecked(i);
        const Parameter*right_param = right_morph_source->params.getUnchecked(i);

        // VALUE
        const float new_value = (left_param->get_value()*(1.0f - power_of_right_)) + (right_param->get_value() * power_of_right_);
        target_param->set_value_without_notification( new_value );

        // MODULATION VALUE
        if( has_modulation( target_param ) )
        {
            const float target_modulation = (left_param->get_modulation_amount()*(1.0f - power_of_right_)) + (right_param->get_modulation_amount()*power_of_right_);
            target_param->set_modulation_amount_without_notification( target_modulation );
        }
    }
    last_power_of_right = power_of_right_;
}
void MorphGroup::morph_switchs( bool left_right_ ) noexcept
{
    current_switch = left_right_;
    for( int i = 0 ; i != switch_bool_params.size() ; ++i )
    {
        if( current_switch == RIGHT )
            switch_bool_params[i]->set_value_without_notification( right_morph_source->switch_bool_params[i]->get_value() );
        else
            switch_bool_params[i]->set_value_without_notification( left_morph_source->switch_bool_params[i]->get_value() );
    }
    for( int i = 0 ; i != switch_int_params.size() ; ++i )
    {
        if( current_switch == RIGHT )
            switch_int_params[i]->set_value_without_notification( right_morph_source->switch_int_params[i]->get_value() );
        else
            switch_int_params[i]->set_value_without_notification( left_morph_source->switch_int_params[i]->get_value() );
    }
}
#define SYNC_MORPH_STEPS 100
#define SYNC_MORPH_TIME 10
void MorphGroup::run_sync_morph() noexcept
{
    stopTimer();
    current_callbacks = 0;

    sync_param_deltas.clearQuick();
    sync_modulation_deltas.clearQuick();
    for( int i = 0 ; i != params.size() ; ++i )
    {
        Parameter*target_param = params.getUnchecked(i);
        const Parameter*left_param = left_morph_source->params.getUnchecked(i);
        const Parameter*right_param = right_morph_source->params.getUnchecked(i);

        // VALUE
        {
            // bei 0.5 = (l + r)/2
            const float target_value = (left_param->get_value()*(1.0f - last_power_of_right )) + (right_param->get_value()*last_power_of_right);
            const float current_value = target_param->get_value();
            sync_param_deltas.add( (target_value-current_value)/SYNC_MORPH_STEPS );
        }

        // MODULATION
        if( has_modulation( target_param ) )
        {
            const float target_modulation = (left_param->get_modulation_amount()*(1.0f - last_power_of_right )) + (right_param->get_modulation_amount()*last_power_of_right);
            const float current_modulation = target_param->get_modulation_amount();
            sync_modulation_deltas.add( (target_modulation-current_modulation)/SYNC_MORPH_STEPS );
        }
        else
        {
            sync_modulation_deltas.add( -1 );
        }
    }

    startTimer(SYNC_MORPH_TIME);
}
void MorphGroup::timerCallback()
{
    for( int i = 0 ; i != params.size() ; ++i )
    {
        Parameter*param = params.getUnchecked(i);

        // VALUE
        {
            const ParameterInfo& info = param->get_info();
            const float min = info.min_value;
            const float max = info.max_value;
            float new_value = param->get_value() + sync_param_deltas[i];
            if( new_value > max )
            {
                new_value = max;
            }
            else if( new_value < min )
            {
                new_value = min;
            }

            param->set_value_without_notification( new_value );
        }

        // MODULATION
        const float modulation_delta = sync_modulation_deltas[i];
        if( modulation_delta != -1 )
        {
            float new_modualtation = param->get_modulation_amount() + modulation_delta;
            if( new_modualtation > 1 )
            {
                new_modualtation = 1;
            }
            else if( new_modualtation < -1 )
            {
                new_modualtation = -1;
            }

            param->set_modulation_amount_without_notification( new_modualtation );
        }
    }

    if( current_callbacks++ == SYNC_MORPH_STEPS )
    {
        stopTimer();
        morph(last_power_of_right);
    }
}


//==============================================================================
void MorphGroup::parameter_value_changed( Parameter* param_ ) noexcept
{
    // SUPPORT FOR INT AND BOOL DIABLED
    /*
    TYPES_DEF type = type_of( param_ );
    if( type == IS_BOOL )
    {
        const int param_id = switch_bool_params.indexOf( reinterpret_cast< BoolParameter* >( param_ ) );
        if( param_id != -1 )
        {
            if( current_switch == LEFT )
                left_morph_source->switch_bool_params[param_id]->set_value_without_notification( param_->get_value() );
            else
                right_morph_source->switch_bool_params[param_id]->set_value_without_notification( param_->get_value() );
        }
    }
    else if( type == IS_INT )
    {
        const int param_id = switch_int_params.indexOf( reinterpret_cast< IntParameter* >( param_ ) );
        if( param_id != -1 )
        {
            if( current_switch == LEFT )
                left_morph_source->switch_int_params[param_id]->set_value_without_notification( param_->get_value() );
            else
                right_morph_source->switch_int_params[param_id]->set_value_without_notification( param_->get_value() );
        }
    }
    else
    */
    {
        const int param_id = params.indexOf( param_ );
        if( param_id != -1 )
        {
            Parameter*const left_source_param = left_morph_source->params[param_id];
            Parameter*const right_source_param = right_morph_source->params[param_id];

            const double right_power = last_power_of_right;
            const double left_power = 1.0f-right_power;

            // KEEP THE RIGHT SIDE UNTOUCHED
            if( left_power == 1 )
            {
                left_source_param->set_value_without_notification( param_->get_value() );
            }
            // KEEP THE LEFT SIDE UNTOUCHED
            else if( right_power == 1 )
            {
                right_source_param->set_value_without_notification( param_->get_value() );
            }
            else
            {
                const ParameterInfo& info = param_->get_info();
                const float max = info.max_value;
                const float min = info.min_value;

                // ---------------
                // 8a + 4b = 12 | - 8a
                // 4b = 12 - 8a | :4
                // b = 3 - 2a
                // -------------------------
                // left_power*left_value + right_power*right_value = current_value | - left_power*left_value
                // right_power*right_value = current_value - left_power*left_value | :right_power
                // right_value = (current_value/right_power) - (left_power/right_power)*left_value
                // -------------------------
                // left_power*left_value + right_power*right_value = current_value | - right_power*right_value
                // left_power*left_value = current_value - right_power*right_value | :left_power
                // left_value = (current_value/left_power) - (right_power/left_power)*right_value

                const float current_value = param_->get_value();
                const float right_value = right_source_param->get_value();
                const float left_value = left_source_param->get_value();
                float new_left;
                float new_right;
                if( left_power >= right_power )
                {
                    new_left = (current_value/left_power) - (right_power/left_power)*right_value;
                    if( new_left < min )
                    {
                        new_left = min;
                        new_right = (current_value/right_power) - (left_power/right_power)*new_left;
                    }
                    else if( new_left > max )
                    {
                        new_left = max;
                        new_right = (current_value/right_power) - (left_power/right_power)*new_left;
                    }
                    else
                    {
                        new_right = right_value;
                    }
                }
                else
                {
                    new_right = (current_value/right_power) - (left_power/right_power)*left_value;
                    if( new_right < min )
                    {
                        new_right = min;
                        new_left = (current_value/left_power) - (right_power/left_power)*new_right;
                    }
                    else if( new_left > max )
                    {
                        new_right = max;
                        new_left = (current_value/left_power) - (right_power/left_power)*new_right;
                    }
                    else
                    {
                        new_left = left_value;
                    }
                }
                left_source_param->set_value_without_notification( new_left );
                right_source_param->set_value_without_notification( new_right );
                jassert( current_value != left_power*left_source_param->get_value() + right_power*right_source_param->get_value() );
            }
        }
    }
}
void MorphGroup::parameter_modulation_value_changed( Parameter* param_ ) noexcept
{
    const int param_id = params.indexOf( param_ );
    if( param_id != -1 )
    {
        Parameter*const left_source_param = left_morph_source->params[param_id];
        Parameter*const right_source_param = right_morph_source->params[param_id];

        const double right_power = last_power_of_right;
        const double left_power = 1.0f-right_power;

        // KEEP THE RIGHT SIDE UNTOUCHED
        if( left_power == 1 )
        {
            left_source_param->set_modulation_amount_without_notification( param_->get_modulation_amount() );
        }
        // KEEP THE LEFT SIDE UNTOUCHED
        else if( right_power == 1 )
        {
            right_source_param->set_modulation_amount_without_notification( param_->get_modulation_amount() );
        }
        else
        {
            const ParameterInfo& info = param_->get_info();
            const float max = 1;
            const float min = -1;

            // ---------------
            // 8a + 4b = 12 | - 8a
            // 4b = 12 - 8a | :4
            // b = 3 - 2a
            // -------------------------
            // left_power*left_value + right_power*right_value = current_value | - left_power*left_value
            // right_power*right_value = current_value - left_power*left_value | :right_power
            // right_value = (current_value/right_power) - (left_power/right_power)*left_value
            // -------------------------
            // left_power*left_value + right_power*right_value = current_value | - right_power*right_value
            // left_power*left_value = current_value - right_power*right_value | :left_power
            // left_value = (current_value/left_power) - (right_power/left_power)*right_value

            const float current_value = param_->get_modulation_amount();
            const float right_value = right_source_param->get_modulation_amount();
            const float left_value = left_source_param->get_modulation_amount();
            float new_left;
            float new_right;
            if( left_power >= right_power )
            {
                new_left = (current_value/left_power) - (right_power/left_power)*right_value;
                if( new_left < min )
                {
                    new_left = min;
                    new_right = (current_value/right_power) - (left_power/right_power)*new_left;
                }
                else if( new_left > max )
                {
                    new_left = max;
                    new_right = (current_value/right_power) - (left_power/right_power)*new_left;
                }
                else
                {
                    new_right = right_value;
                }
            }
            else
            {
                new_right = (current_value/right_power) - (left_power/right_power)*left_value;
                if( new_right < min )
                {
                    new_right = min;
                    new_left = (current_value/left_power) - (right_power/left_power)*new_right;
                }
                else if( new_left > max )
                {
                    new_right = max;
                    new_left = (current_value/left_power) - (right_power/left_power)*new_right;
                }
                else
                {
                    new_left = left_value;
                }
            }
            left_source_param->set_modulation_amount_without_notification( new_left );
            right_source_param->set_modulation_amount_without_notification( new_right );
            jassert( current_value != left_power*left_source_param->get_modulation_amount() + right_power*right_source_param->get_modulation_amount() );
        }
    }
}

//==============================================================================
//==============================================================================
//==============================================================================
COLD void set_default_midi_assignments( MoniqueSynthData& synth_data, MoniqueAudioProcessor*const midi_device_manager_ ) noexcept
{
    MIDIControl* midi_control;

    OSCData& master_osc_data( *synth_data.osc_datas[MASTER_OSC] );
    FMOscData& fm_osc_data( *synth_data.fm_osc_data );
    OSCData& osc_data_2( *synth_data.osc_datas[1] );
    OSCData& osc_data_3( *synth_data.osc_datas[2] );
    LFOData& master_lfo_data( *synth_data.lfo_datas[MASTER_OSC] );
    LFOData& lfo_data_2( *synth_data.lfo_datas[1] );
    LFOData& lfo_data_3( *synth_data.lfo_datas[2] );
    ENVData& main_env_data( *synth_data.env_data );
    FilterData& filter_data_1( *synth_data.filter_datas[0] );
    FilterData& filter_data_2( *synth_data.filter_datas[1] );
    FilterData& filter_data_3( *synth_data.filter_datas[2] );
    ReverbData& reverb_data( *synth_data.reverb_data );
    ChorusData& chorus_data( *synth_data.chorus_data );

    /*
    // 0 Bank Select // FIX!
    // 1
    synth_data.glide.midi_control->train( 1, nullptr, midi_device_manager_ );
    // 2 UNUSED
    main_env_data.sustain.midi_control->train( 1, nullptr, midi_device_manager_ );
    // 3 UNUSED
    // 4
    synth_data.ctrl.midi_control->train( 4, nullptr, midi_device_manager_ );
    // 5
    synth_data.velocity_glide_time.midi_control->train( 5, nullptr, midi_device_manager_ );
    // 6 UNUSED
    // 7
    synth_data.volume.midi_control->train( 7, nullptr, midi_device_manager_ );
    // 8 UNUSED
    // 9 UNUSED
    // 10 UNUSED
    // 11 UNUSED
    // 12
    synth_data.shape.midi_control->train( 12, nullptr, midi_device_manager_ );
    // 13
    // TODO synth_data.curve_shape.midi_control->train( 13, nullptr );
    // 14
    synth_data.effect_bypass.midi_control->train( 14, nullptr, midi_device_manager_ );
    // 15
    synth_data.linear_morhp_state.midi_control->train( 15, nullptr, midi_device_manager_ );
    // 16
    synth_data.morhp_states[0].midi_control->train( 16, nullptr, midi_device_manager_ );
    // 17
    synth_data.morhp_states[1].midi_control->train( 17, nullptr, midi_device_manager_ );
    // 18
    synth_data.morhp_states[2].midi_control->train( 18, nullptr, midi_device_manager_ );
    // 19
    synth_data.morhp_states[3].midi_control->train( 19, nullptr, midi_device_manager_ );
    // 20
    // TODO fm_osc_data.puls_width.midi_control->train( 20, nullptr );
    // 21
    // TODO master_osc_data.fm_mix_algorythm.midi_control->train( 21, nullptr );
    // 22
    filter_data_1.input_sustains[0].midi_control->train( 22, nullptr, midi_device_manager_ );
    // 23
    filter_data_1.input_sustains[1].midi_control->train( 23, nullptr, midi_device_manager_ );
    // 24
    filter_data_1.input_sustains[2].midi_control->train( 24, nullptr, midi_device_manager_ );
    // 25
    filter_data_1.env_data->attack.midi_control->train( 25, nullptr, midi_device_manager_ );
    // 26
    filter_data_1.env_data->decay.midi_control->train( 26, nullptr, midi_device_manager_ );
    // 27
    filter_data_1.env_data->sustain.midi_control->train( 27, nullptr, midi_device_manager_ );
    // 28
    filter_data_1.env_data->release.midi_control->train( 28, nullptr, midi_device_manager_ );
    // 29
    filter_data_1.adsr_lfo_mix.midi_control->train( 29, nullptr, midi_device_manager_ );
    // 30
    master_lfo_data.speed.midi_control->train( 30, nullptr, midi_device_manager_ );
    // 31
    filter_data_1.cutoff.midi_control->train( 31, nullptr, midi_device_manager_ );
    // 32
    filter_data_1.resonance.midi_control->train( 32, nullptr, midi_device_manager_ );
    // 33
    // TODO filter_data_1.gain.midi_control->train( 33, nullptr, midi_device_manager_ );
    // 34
    filter_data_1.distortion.midi_control->train( 34, nullptr, midi_device_manager_ );
    // 35 UNUSED / SPACER
    // 36
    filter_data_2.input_sustains[0].midi_control->train( 36, nullptr, midi_device_manager_ );
    // 37
    filter_data_2.input_sustains[1].midi_control->train( 37, nullptr, midi_device_manager_ );
    // 38
    filter_data_2.input_sustains[2].midi_control->train( 38, nullptr, midi_device_manager_ );
    // 39
    filter_data_2.env_data->attack.midi_control->train( 39, nullptr, midi_device_manager_ );
    // 40
    filter_data_2.env_data->decay.midi_control->train( 40, nullptr, midi_device_manager_ );
    // 41
    filter_data_2.env_data->sustain.midi_control->train( 41, nullptr, midi_device_manager_ );
    // 42
    filter_data_2.env_data->release.midi_control->train( 42, nullptr, midi_device_manager_ );
    // 43
    filter_data_2.adsr_lfo_mix.midi_control->train( 43, nullptr, midi_device_manager_ );
    // 44
    lfo_data_2.speed.midi_control->train( 44, nullptr, midi_device_manager_ );
    // 45
    filter_data_2.cutoff.midi_control->train( 45, nullptr, midi_device_manager_ );
    // 46
    filter_data_2.resonance.midi_control->train( 46, nullptr, midi_device_manager_ );
    // 47
    // TODO filter_data_2.gain.midi_control->train( 47, nullptr, midi_device_manager_ );
    // 48
    filter_data_2.distortion.midi_control->train( 48, nullptr, midi_device_manager_ );
    // 49 UNUSED / SPACER
    // 50
    filter_data_3.input_sustains[0].midi_control->train( 50, nullptr, midi_device_manager_ );
    // 51
    filter_data_3.input_sustains[1].midi_control->train( 51, nullptr, midi_device_manager_ );
    // 52
    filter_data_3.input_sustains[2].midi_control->train( 52, nullptr, midi_device_manager_ );
    // 53
    filter_data_3.env_data->attack.midi_control->train( 53, nullptr, midi_device_manager_ );
    // 54
    filter_data_3.env_data->decay.midi_control->train( 54, nullptr, midi_device_manager_ );
    // 55
    filter_data_3.env_data->sustain.midi_control->train( 55, nullptr, midi_device_manager_ );
    // 56
    filter_data_3.env_data->release.midi_control->train( 56, nullptr, midi_device_manager_ );
    // 57
    filter_data_3.adsr_lfo_mix.midi_control->train( 57, nullptr, midi_device_manager_ );
    // 58
    lfo_data_3.speed.midi_control->train( 58, nullptr, midi_device_manager_ );
    // 59
    filter_data_3.cutoff.midi_control->train( 59, nullptr, midi_device_manager_ );
    // 60
    filter_data_3.resonance.midi_control->train( 60, nullptr, midi_device_manager_ );
    // 61
    // TODO filter_data_3.gain.midi_control->train( 61, nullptr, midi_device_manager_ );
    // 62
    filter_data_3.distortion.midi_control->train( 62, nullptr, midi_device_manager_ );
    // 63 UNUSED / SPACER
    // 64 Hold Pedal (on/off) // FIX
    // 65 UNUSED
    // 66 Sustenuto Pedal (on/off) // FIX
    // 67 Soft Pedal (on/off) // FIX
    // 68
    // TODO synth_data.force_envs_to_zero.midi_control->train( 68, nullptr );
    // 69 UNUSED
    // 70
    fm_osc_data.fm_freq.midi_control->train( 70, nullptr, midi_device_manager_ );
    // 71
    main_env_data.decay.midi_control->train( 71, nullptr, midi_device_manager_ );
    // 72
    main_env_data.release.midi_control->train( 72, nullptr, midi_device_manager_ );
    // 73
    main_env_data.attack.midi_control->train( 73, nullptr, midi_device_manager_ );
    // 74
    // DISABLED FOR EWIND: main_env_data.sustain.midi_control->train( 74, nullptr );
    // 75
    master_osc_data.fm_amount.midi_control->train( 75, nullptr, midi_device_manager_ );
    // 76
    master_osc_data.wave.midi_control->train( 76, nullptr, midi_device_manager_ );
    // 77
    osc_data_2.fm_amount.midi_control->train( 77, nullptr, midi_device_manager_ );
    // 78
    osc_data_2.wave.midi_control->train( 78, nullptr, midi_device_manager_ );
    // 79
    osc_data_3.fm_amount.midi_control->train( 79, nullptr, midi_device_manager_ );
    // 80
    // TODO master_osc_data.o_mod.midi_control->train( 80, nullptr );
    // 81
    osc_data_2.sync.midi_control->train( 81, nullptr, midi_device_manager_ );
    // 82
    osc_data_3.sync.midi_control->train( 82, nullptr, midi_device_manager_ );
    // 83
    master_osc_data.sync.midi_control->train( 83, nullptr, midi_device_manager_ );
    // 84 UNUSED
    // 85 UNUSED
    // 86 UNUSED
    // 87 UNUSED
    // 88 UNUSED
    main_env_data.sustain.midi_control->train( 1, nullptr, midi_device_manager_ );
    // 89 UNUSED
    // 90 UNUSED
    // 91 Reverb Level
    reverb_data.dry_wet_mix.midi_control->train( 91, nullptr, midi_device_manager_ );
    // 92 Tremolo Level
    // 93 Chorus Level
    chorus_data.modulation.midi_control->train( 93, nullptr, midi_device_manager_ );
    // 94
    master_osc_data.tune.midi_control->train( 94, nullptr, midi_device_manager_ );
    // 95
    synth_data.delay.midi_control->train( 95, nullptr, midi_device_manager_ );
    // 96 UNUSED
    // 97 DUNUSED
    // 98 UNUSED
    // 99 UNUSED
    // 100 UNUSED
    // 101 UNUSED
    // 102 UNUSED
    // 103 UNUSED
    // 104 UNUSED
    // 105 UNUSED
    // 106 UNUSED
    // 107 UNUSED
    // 108 UNUSED
    // 109 UNUSED
    // 110 UNUSED
    // 111 UNUSED
    // 112 UNUSED
    // 113 UNUSED
    // 114 UNUSED
    // 115 UNUSED
    // 116 UNUSED
    // 117 UNUSED
    // 118 UNUSED
    // 119 UNUSED
    // 120 UNUSED
    // 121 UNUSED
    // 122 UNUSED
    // 123 All Notes Off
    // 124 Omni Mode Off
    // TODO
    // 125 Omni Mode On
    // TODO
    // 126 UNUSED
    // 127 UNUSED
    */
};

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
struct CREATE_SIN_LOOKUP
{
    static float* exec() noexcept
    {
        float* table_ = new float[LOOKUP_TABLE_SIZE];
        for(int i = 0; i < LOOKUP_TABLE_SIZE; i++)
        {
            table_[i] = std::sin( double(i) / TABLESIZE_MULTI );
        }

        return table_;
    }
};
//==============================================================================
//==============================================================================
//==============================================================================
struct CREATE_COS_LOOKUP
{
    static float* exec() noexcept
    {
        float* table_ = new float[LOOKUP_TABLE_SIZE];
        for(int i = 0; i < LOOKUP_TABLE_SIZE; i++)
        {
            table_[i] = std::cos( double(i) / TABLESIZE_MULTI );
        }

        return table_;
    }
};
//==============================================================================
//==============================================================================
//==============================================================================
struct CREATE_EXP_LOOKUP
{
    static float* exec() noexcept
    {
        float* table_ = new float[LOOKUP_TABLE_SIZE];
        for(int i = 0; i < LOOKUP_TABLE_SIZE; i++)
        {
#define EXP_PI_05_CORRECTION 4.81048f
#define LOG_PI_1_CORRECTION 1.42108f
#define EXP_PI_1_CORRECTION 23.1407f
            table_[i] = (std::exp( double(i) / TABLESIZE_MULTI ) / EXP_PI_1_CORRECTION);
        }

        return table_;
    }
};

//==============================================================================
//==============================================================================
//==============================================================================
#define SYNTH_DATA_NAME "SD"
COLD MoniqueSynthData::MoniqueSynthData( DATA_TYPES data_type,
        UiLookAndFeel*look_and_feel_,
        MoniqueAudioProcessor*const audio_processor_,
        RuntimeNotifyer*const runtime_notifyer_,
        RuntimeInfo*const info_,
        DataBuffer*data_buffer_,
        SmoothManager*smooth_manager_ ) noexcept
:
ui_look_and_feel( look_and_feel_ ),
                  audio_processor( audio_processor_ ),
                  smooth_manager( data_type == MASTER ? new SmoothManager(runtime_notifyer_) : smooth_manager_ ),
                  runtime_notifyer( runtime_notifyer_ ),
                  runtime_info( info_ ),
                  data_buffer( data_buffer_ ),

                  sine_lookup( data_type == MASTER ? CREATE_SIN_LOOKUP::exec() : nullptr ),
                  cos_lookup( data_type == MASTER ? CREATE_COS_LOOKUP::exec() : nullptr ),
                  exp_lookup( data_type == MASTER ? CREATE_EXP_LOOKUP::exec() : nullptr ),

                  id( data_type ),

                  volume
                  (
                      MIN_MAX( 0, 1 ),
                      0.9,
                      1000,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"volume"),
                      generate_short_human_name("MAIN","volume")
                  ),
                  volume_smoother( smooth_manager ,&volume),
                  glide
                  (
                      MIN_MAX( 0, 1 ),
                      0.05,
                      1000,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"glide"),
                      generate_short_human_name("GLOB","note_glide")
                  ),
                  glide_smoother(smooth_manager ,&glide ),
                  delay
                  (
                      MIN_MAX( 0, 1 ),
                      0,
                      1000,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"delay"),
                      generate_short_human_name("FX","delay")
                  ),
                  delay_smoother(smooth_manager,&delay),
                  delay_pan
                  (
                      MIN_MAX( -1, 1 ),
                      0,
                      2000,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"delay_pan"),
                      generate_short_human_name("FX","delay_pan")
                  ),
                  delay_pan_smoother(smooth_manager,&delay_pan),
                  effect_bypass
                  (
                      MIN_MAX( 0, 1 ),
                      1,
                      1000,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"effect_bypass"),
                      generate_short_human_name("FX","mix")
                  ),
                  effect_bypass_smoother(smooth_manager,&effect_bypass),
                  shape
                  (
                      MIN_MAX( 0, 1 ),
                      0.05,
                      1000,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"shape"),
                      generate_short_human_name("EQ","resonance")
                  ),
                  shape_smoother(smooth_manager,&shape),
                  distortion
                  (
                      MIN_MAX( 0, 1 ),
                      0.6,
                      1000,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"distortion"),
                      generate_short_human_name("FX","destroy")
                  ),
                  distortion_smoother(smooth_manager,&distortion),
                  octave_offset
                  (
                      MIN_MAX( -2, 2 ),
                      0,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"octave_offset"),
                      generate_short_human_name("GLOB","octave")
                  ),
                  note_offset
                  (
                      MIN_MAX( 0, 12 ),
                      0,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"arp_note_offset"),
                      generate_short_human_name("GLOB","note_offset")
                  ),

                  sync
                  (
                      true,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"sync"),
                      generate_short_human_name("GLOB","speed_sync")
                  ),
                  speed
                  (
                      MIN_MAX( 20, 1000 ),
                      128,
                      980*10,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"speed"),
                      generate_short_human_name("GLOB","speed")
                  ),

                  glide_motor_time
                  (
                      MIN_MAX( 1, 1000 ),
                      50,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"smooth_motor_time"),
                      generate_short_human_name("GLOB","smooth_motor_time")
                  ),
                  velocity_glide_time
                  (
                      MIN_MAX( 1, 999 ),
                      30,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"velocity_glide_time"),
                      generate_short_human_name("GLOB","velocity_glide")
                  ),

                  ctrl
                  (
                      false,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"ctrl"),
                      generate_short_human_name("GLOB","ctrl")
                  ),
                  midi_pickup_offset
                  (
                      MIN_MAX( 0, 1 ),
                      1,
                      1000,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"midi_pickup_offset"),
                      generate_short_human_name("MIDI","cc_pick_up")
                  ),

// -------------------------------------------------------------
                  osci_show_osc_1
                  (
                      true,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_osc_1"),
                      generate_short_human_name("CONF","osci_show_osc_1")
                  ),
                  osci_show_osc_2
                  (
                      true,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_osc_2"),
                      generate_short_human_name("CONF","osci_show_osc_2")
                  ),
                  osci_show_osc_3
                  (
                      true,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_osc_3"),
                      generate_short_human_name("CONF","osci_show_osc_3")
                  ),
                  osci_show_flt_env_1
                  (
                      false,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_flt_env_1"),
                      generate_short_human_name("CONF","osci_show_flt_env_1")
                  ),
                  osci_show_flt_env_2
                  (
                      false,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_flt_env_2"),
                      generate_short_human_name("CONF","osci_show_flt_env_2")
                  ),
                  osci_show_flt_env_3
                  (
                      false,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_flt_env_3"),
                      generate_short_human_name("CONF","osci_show_flt_env_3")
                  ),
                  osci_show_flt_1
                  (
                      true,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_flt_1"),
                      generate_short_human_name("CONF","osci_show_flt_1")
                  ),
                  osci_show_flt_2
                  (
                      true,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_flt_2"),
                      generate_short_human_name("CONF","osci_show_flt_2")
                  ),
                  osci_show_flt_3
                  (
                      true,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_flt_3"),
                      generate_short_human_name("CONF","osci_show_flt_3")
                  ),
                  osci_show_eq
                  (
                      false,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_eq"),
                      generate_short_human_name("CONF","osci_show_eq")
                  ),
                  osci_show_out
                  (
                      true,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_out"),
                      generate_short_human_name("CONF","osci_show_out")
                  ),
                  osci_show_out_env
                  (
                      false,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_out_env"),
                      generate_short_human_name("CONF","osci_show_out_env")
                  ),
                  osci_show_range
                  (
                      MIN_MAX( 0, 1 ),
                      0.05,
                      100,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"osci_show_range"),
                      generate_short_human_name("CONF","osci_show_range")
                  ),

// -------------------------------------------------------------
                  auto_close_env_popup
                  (
                      true,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"auto_close_env_popup"),
                      generate_short_human_name("POP","auto_close_env_popup")
                  ),
                  auto_switch_env_popup
                  (
                      true,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"auto_switch_env_popup"),
                      generate_short_human_name("POP","auto_switch_env_popup")
                  ),

// -------------------------------------------------------------
                  is_osci_open
                  (
                      false,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"is_osci_open"),
                      generate_short_human_name("CONF","is_osci_open")
                  ),

// -------------------------------------------------------------
                  keep_arp_always_on
                  (
                      false,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"arp_ON_always"),
                      generate_short_human_name("GLOB","arp_ON_always")
                  ),
                  keep_arp_always_off
                  (
                      false,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"arp_OFF_always"),
                      generate_short_human_name("GLOB","arp_OFF_always")
                  ),


// -------------------------------------------------------------
                  num_extra_threads
                  (
                      MIN_MAX( 0, THREAD_LIMIT ),
                      0,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"cpus"),
                      generate_short_human_name("CONF","cpus")
                  ),

// -------------------------------------------------------------
                  animate_envs
                  (
                      true,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"animate_envs"),
                      generate_short_human_name("CONF","animate_envs")
                  ),
                  show_tooltips
                  (
                      true,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"show_tooltips"),
                      generate_short_human_name("CONF","show_tooltips")
                  ),
                  bind_sustain_and_sostenuto_pedal
                  (
                      false,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"bind_pedals"),
                      generate_short_human_name("MIDI","bind_pedals")
                  ),
                  sliders_in_rotary_mode
                  (
                      false,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"slider_rotary"),
                      generate_short_human_name("CONF","slider_type")
                  ),
                  sliders_sensitivity
                  (
                      MIN_MAX( 100, 2000 ),
                      500,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"slider_sensitivity"),
                      generate_short_human_name("CONF","slider_sensitivity")
                  ),
                  ui_scale_factor
                  (
                      MIN_MAX( 0.6, 10 ),
                      0.7,
                      1000,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"ui_scale_factor"),
                      generate_short_human_name("CONF","ui_scale_factor")
                  ),

// ----
                  env_data( new ENVData( smooth_manager, MAIN_ENV ) ),
                  eq_data(new EQData( smooth_manager, MASTER )),
                  arp_sequencer_data(new ArpSequencerData( MASTER )),
                  reverb_data(new ReverbData( smooth_manager, MASTER ) ),
                  chorus_data(new ChorusData( smooth_manager, MASTER )),

// MORPH
// -------------------------------------------------------------
                  morhp_states
                  (
                      SUM_MORPHER_GROUPS,

                      MIN_MAX( 0, 1 ),
                      0,
                      1000,

                      SYNTH_DATA_NAME,SYNTH_DATA_NAME,
                      MASTER,
                      "morph_state","morph",false
                  ),
                  is_morph_modulated
                  (
                      SUM_MORPHER_GROUPS,

                      false,

                      SYNTH_DATA_NAME,SYNTH_DATA_NAME,
                      MASTER,
                      "is_morph_modulated","is_morph_mod",false
                  ),
                  morhp_automation_power
                  (
                      SUM_MORPHER_GROUPS,

                      MIN_MAX( 0, 1 ),
                      0,
                      1000,

                      SYNTH_DATA_NAME,SYNTH_DATA_NAME,
                      MASTER,
                      "mfo_power","morph_mod_power",false
                  ),
                  morhp_switch_states
                  (
                      SUM_MORPHER_GROUPS,

                      LEFT,

                      SYNTH_DATA_NAME,SYNTH_DATA_NAME,
                      MASTER,
                      "morph_switch_state","morph_tgl",false
                  ),
                  linear_morhp_state
                  (
                      MIN_MAX( 0, 3 ),
                      0,
                      100,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"linear_morhp_state"),
                      generate_short_human_name("GLOB","morph_line")
                  ),
                  morph_motor_time
                  (
                      MIN_MAX( 20, 20000 ),
                      1000,
                      generate_param_name(SYNTH_DATA_NAME,MASTER,"morph_motor_time"),
                      generate_short_human_name("CONF","morph_motor")
                  ),
                  morph_group_1( new MorphGroup() ),
                  morph_group_2( new MorphGroup() ),
                  morph_group_3( new MorphGroup() ),
                  morph_group_4( new MorphGroup() ),

// FILES
// ----
                  current_program(-1),
                  current_program_abs(-1),
                  current_bank(0),

                  error_string("ERROR"),
                  alternative_program_name("NO PROGRAM SELECTED")
{
    // OSCS DATA
    fm_osc_data = new FMOscData(smooth_manager);
    for( int i = 0 ; i != SUM_OSCS ; ++i )
    {
        OSCData* data = new OSCData(smooth_manager,i);
        osc_datas.add( data );
    }
    osc_datas.minimiseStorageOverheads();

    // LFO DATA
    for( int i = 0 ; i != SUM_LFOS ; ++i )
    {
        LFOData* data = new LFOData(smooth_manager,i);
        lfo_datas.add( data );
    }
    lfo_datas.minimiseStorageOverheads();

    // MFO DATA
    for( int i = 0 ; i != SUM_MORPHER_GROUPS ; ++i )
    {
        MFOData* data = new MFOData(smooth_manager,i);
        mfo_datas.add( data );
    }
    mfo_datas.minimiseStorageOverheads();

    // FILTERS
    for( int i = 0 ; i != SUM_FILTERS ; ++i )
    {
        FilterData* filter_data = new FilterData(smooth_manager,i);
        filter_datas.add( filter_data );
    }
    filter_datas.minimiseStorageOverheads();

    // MORPH STUFF
    init_morph_groups( data_type );

    // FILE HANDLING
    colect_saveable_parameters();

    if( data_type == MASTER )
    {
        colect_global_parameters();
        all_parameters.addArray( saveable_parameters );
        all_parameters.addArray( global_parameters );
        all_parameters.add( &ctrl );
        all_parameters.add( &linear_morhp_state );

        refresh_banks_and_programms( *this );
        set_default_midi_assignments( *this, audio_processor_ );
    }
}
#include "monique_core_Synth.h"
COLD MoniqueSynthData::~MoniqueSynthData() noexcept
{
    eq_data = nullptr;
    arp_sequencer_data = nullptr;
    reverb_data = nullptr;
    chorus_data = nullptr;

    lfo_datas.clear();
    osc_datas.clear();
    filter_datas.clear();

    delete [] exp_lookup;
    delete [] cos_lookup;
    delete [] sine_lookup;
}

//==============================================================================
static inline void copy( MoniqueSynthData* dest_, const MoniqueSynthData* src_ ) noexcept
{
    dest_->volume = src_->volume;
    dest_->glide = src_->glide;
    dest_->delay = src_->delay;
    dest_->delay_pan = src_->delay_pan;
    dest_->effect_bypass = src_->effect_bypass;
    dest_->speed = src_->speed;
    dest_->glide_motor_time = src_->glide_motor_time;
    dest_->velocity_glide_time = src_->velocity_glide_time;
    dest_->sync = src_->sync;
    dest_->distortion = src_->distortion;
    dest_->shape = src_->shape;
    dest_->octave_offset = src_->octave_offset;
    dest_->note_offset = src_->note_offset;

    for( int i = 0 ; i != SUM_MORPHER_GROUPS ; ++i )
    {
        dest_->is_morph_modulated[i].set_value( src_->is_morph_modulated[i].get_value());
    }

    for( int i = 0 ; i != SUM_LFOS ; ++i )
    {
        copy( dest_->lfo_datas[i], src_->lfo_datas[i] );
    }

    copy( dest_->fm_osc_data, src_->fm_osc_data );
    for( int i = 0 ; i != SUM_OSCS ; ++i )
    {
        copy( dest_->osc_datas[i], src_->osc_datas[i] );
    }

    for( int i = 0 ; i != SUM_FILTERS ; ++i )
    {
        copy( dest_->filter_datas[i], src_->filter_datas[i] );
    }

    copy( dest_->env_data, src_->env_data );
    copy( dest_->eq_data, src_->eq_data );
    copy( dest_->arp_sequencer_data, src_->arp_sequencer_data );
    copy( dest_->reverb_data, src_->reverb_data );
    copy( dest_->chorus_data, src_->chorus_data );

    // NO NEED FOR COPY
    // morhp_states
}
COLD void MoniqueSynthData::colect_saveable_parameters() noexcept
{
    // on top to be the first on load and get the right update order (bit hacky, but ok ;--)

    for( int i = 0 ; i != SUM_OSCS ;  ++i )
    {
        collect_saveable_parameters( osc_datas[i], saveable_parameters );
    }
    collect_saveable_parameters( fm_osc_data, saveable_parameters );
    for( int i = 0 ; i != SUM_LFOS ; ++i )
    {
        collect_saveable_parameters( lfo_datas[i], saveable_parameters );
        saveable_parameters.add( &is_morph_modulated[i] );
    }
    for( int i = 0 ; i != SUM_MORPHER_GROUPS ; ++i )
    {
        collect_saveable_parameters( mfo_datas[i], saveable_parameters );
    }
    for( int flt_id = 0 ; flt_id != SUM_FILTERS ; ++flt_id )
    {
        collect_saveable_parameters( filter_datas[flt_id], saveable_parameters );
    }

    for( int morpher_id = 0 ; morpher_id != SUM_MORPHER_GROUPS ; ++morpher_id )
    {
        saveable_parameters.add( &this->morhp_states[morpher_id] );
        saveable_parameters.add( &this->morhp_switch_states[morpher_id] );
    }

    saveable_parameters.add( &this->shape );
    saveable_parameters.add( &this->distortion );
    saveable_parameters.add( &this->delay );
    saveable_parameters.add( &this->delay_pan );
    collect_saveable_parameters( reverb_data, saveable_parameters );
    collect_saveable_parameters( chorus_data, saveable_parameters );
    saveable_parameters.add( &this->effect_bypass );
    collect_saveable_parameters( env_data, saveable_parameters );
    collect_saveable_parameters( eq_data, saveable_parameters );
    saveable_parameters.add( &this->volume );

    saveable_parameters.add( &this->glide );
    saveable_parameters.add( &this->velocity_glide_time );
    collect_saveable_parameters( arp_sequencer_data, saveable_parameters );
    saveable_parameters.add( &this->sync );
    saveable_parameters.add( &this->speed );
    saveable_parameters.add( &this->octave_offset );
    saveable_parameters.add( &this->note_offset );


    saveable_parameters.minimiseStorageOverheads();
}

COLD void MoniqueSynthData::colect_global_parameters() noexcept
{
    global_parameters.add( &osci_show_osc_1 );
    global_parameters.add( &osci_show_osc_2 );
    global_parameters.add( &osci_show_osc_3 );
    global_parameters.add( &osci_show_flt_env_1 );
    global_parameters.add( &osci_show_flt_env_2 );
    global_parameters.add( &osci_show_flt_env_3 );
    global_parameters.add( &osci_show_flt_1 );
    global_parameters.add( &osci_show_flt_2 );
    global_parameters.add( &osci_show_flt_3 );
    global_parameters.add( &osci_show_eq );
    global_parameters.add( &osci_show_out );
    global_parameters.add( &osci_show_out_env );
    global_parameters.add( &osci_show_range );
    global_parameters.add( &num_extra_threads );

    global_parameters.add( &auto_close_env_popup );
    global_parameters.add( &auto_switch_env_popup );

    global_parameters.add( &animate_envs );
    global_parameters.add( &show_tooltips );
    global_parameters.add( &bind_sustain_and_sostenuto_pedal );

    global_parameters.add( &sliders_in_rotary_mode );
    global_parameters.add( &sliders_sensitivity );

    global_parameters.add( &ui_scale_factor );

    global_parameters.add( &midi_pickup_offset );
    global_parameters.add( &ctrl );

    global_parameters.add( &glide_motor_time );
    global_parameters.add( &morph_motor_time );

    global_parameters.minimiseStorageOverheads();
}

//==============================================================================
//==============================================================================
//==============================================================================
COLD void MoniqueSynthData::init_morph_groups( DATA_TYPES data_type ) noexcept
{
    left_morph_source_names.add( "FACTORY DEFAULT" );
    left_morph_source_names.add( "FACTORY DEFAULT" );
    left_morph_source_names.add( "FACTORY DEFAULT" );
    left_morph_source_names.add( "FACTORY DEFAULT" );
    right_morph_source_names.add( "FACTORY DEFAULT" );
    right_morph_source_names.add( "FACTORY DEFAULT" );
    right_morph_source_names.add( "FACTORY DEFAULT" );
    right_morph_source_names.add( "FACTORY DEFAULT" );
    {
        // OSC'S
        {
            {
                morph_group_1->register_parameter( osc_datas[0]->wave.ptr(), data_type == MASTER );
                morph_group_1->register_parameter( fm_osc_data->master_shift.ptr(), data_type == MASTER );
                morph_group_1->register_parameter( osc_datas[0]->fm_amount.ptr(), data_type == MASTER );

                morph_group_1->register_switch_parameter( osc_datas[0]->is_lfo_modulated.bool_ptr(), data_type == MASTER );
                morph_group_1->register_switch_parameter( osc_datas[0]->sync.bool_ptr(), data_type == MASTER );
            }
            {
                morph_group_1->register_parameter( osc_datas[1]->wave.ptr(), data_type == MASTER );
                morph_group_1->register_parameter( osc_datas[1]->tune.ptr(), data_type == MASTER );
                morph_group_1->register_parameter( osc_datas[1]->fm_amount.ptr(), data_type == MASTER );

                morph_group_1->register_switch_parameter( osc_datas[1]->is_lfo_modulated.bool_ptr(), data_type == MASTER );
                morph_group_1->register_switch_parameter( osc_datas[1]->sync.bool_ptr(), data_type == MASTER );
            }
            {
                morph_group_1->register_parameter( osc_datas[2]->wave.ptr() , data_type == MASTER  );
                morph_group_1->register_parameter( osc_datas[2]->tune.ptr(), data_type == MASTER  );
                morph_group_1->register_parameter( osc_datas[2]->fm_amount.ptr(), data_type == MASTER  );

                morph_group_1->register_switch_parameter( osc_datas[2]->is_lfo_modulated.bool_ptr(), data_type == MASTER  );
                morph_group_1->register_switch_parameter( osc_datas[2]->sync.bool_ptr(), data_type == MASTER  );
            }
        }

        // FM
        {
            morph_group_1->register_parameter( fm_osc_data->fm_freq.ptr(), data_type == MASTER );
            morph_group_1->register_parameter( fm_osc_data->fm_swing.ptr(), data_type == MASTER );
            morph_group_1->register_parameter( fm_osc_data->fm_shape.ptr(), data_type == MASTER );
        }

        // FILTERS
        {
            {
                // FLT
                morph_group_2->register_parameter( filter_datas[0]->adsr_lfo_mix.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[0]->distortion.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[0]->cutoff.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[0]->resonance.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[0]->output.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[0]->pan.ptr(), data_type == MASTER  );
                for( int input_id = 0 ; input_id != SUM_INPUTS_PER_FILTER ; ++input_id )
                {
                    morph_group_2->register_parameter( filter_datas[0]->input_envs[input_id]->attack.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[0]->input_envs[input_id]->decay.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[0]->input_envs[input_id]->sustain.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[0]->input_envs[input_id]->sustain_time.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[0]->input_envs[input_id]->release.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[0]->input_envs[input_id]->shape.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[0]->input_sustains[input_id].ptr(), data_type == MASTER  );
                }

                morph_group_2->register_switch_parameter( filter_datas[0]->filter_type.int_ptr(), data_type == MASTER  );
                morph_group_2->register_switch_parameter( filter_datas[0]->modulate_distortion.bool_ptr(), data_type == MASTER  );
                morph_group_2->register_switch_parameter( filter_datas[0]->modulate_cutoff.bool_ptr(), data_type == MASTER  );
                morph_group_2->register_switch_parameter( filter_datas[0]->modulate_resonance.bool_ptr(), data_type == MASTER  );
                for( int input_id = 0 ; input_id != SUM_INPUTS_PER_FILTER ; ++input_id )
                {
                    morph_group_2->register_switch_parameter( reinterpret_cast< BoolParameter* >( filter_datas[0]->input_holds[input_id].ptr() ), data_type == MASTER  );
                }
                morph_group_2->register_switch_parameter( filter_datas[0]->modulate_output.bool_ptr(), data_type == MASTER  );

                // LFO
                morph_group_2->register_parameter( lfo_datas[0]->speed.ptr(), data_type == MASTER  );

                // ENV
                morph_group_2->register_parameter( filter_datas[0]->env_data->attack.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[0]->env_data->decay.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[0]->env_data->sustain.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[0]->env_data->sustain_time.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[0]->env_data->release.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[0]->env_data->shape.ptr(), data_type == MASTER  );
            }

            {
                // FLT
                morph_group_2->register_parameter( filter_datas[1]->adsr_lfo_mix.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[1]->distortion.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[1]->cutoff.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[1]->resonance.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[1]->output.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[1]->pan.ptr(), data_type == MASTER  );
                for( int input_id = 0 ; input_id != SUM_INPUTS_PER_FILTER ; ++input_id )
                {
                    morph_group_2->register_parameter( filter_datas[1]->input_envs[input_id]->attack.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[1]->input_envs[input_id]->decay.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[1]->input_envs[input_id]->sustain.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[1]->input_envs[input_id]->sustain_time.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[1]->input_envs[input_id]->release.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[1]->input_envs[input_id]->shape.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[1]->input_sustains[input_id].ptr(), data_type == MASTER  );
                }

                morph_group_2->register_switch_parameter( filter_datas[1]->filter_type.int_ptr(), data_type == MASTER  );
                morph_group_2->register_switch_parameter( filter_datas[1]->modulate_distortion.bool_ptr(), data_type == MASTER  );
                morph_group_2->register_switch_parameter( filter_datas[1]->modulate_cutoff.bool_ptr(), data_type == MASTER  );
                morph_group_2->register_switch_parameter( filter_datas[1]->modulate_resonance.bool_ptr(), data_type == MASTER  );
                for( int input_id = 0 ; input_id != SUM_INPUTS_PER_FILTER ; ++input_id )
                {
                    morph_group_2->register_switch_parameter( reinterpret_cast< BoolParameter* >( filter_datas[1]->input_holds[input_id].ptr() ), data_type == MASTER  );
                }
                morph_group_2->register_switch_parameter( filter_datas[1]->modulate_output.bool_ptr(), data_type == MASTER  );

                // LFO
                morph_group_2->register_parameter( lfo_datas[1]->speed.ptr(), data_type == MASTER  );

                // ENV
                morph_group_2->register_parameter( filter_datas[1]->env_data->attack.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[1]->env_data->decay.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[1]->env_data->sustain.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[1]->env_data->sustain_time.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[1]->env_data->release.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[1]->env_data->shape.ptr(), data_type == MASTER  );
            }

            {
                // FLT
                morph_group_2->register_parameter( filter_datas[2]->adsr_lfo_mix.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[2]->distortion.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[2]->cutoff.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[2]->resonance.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[2]->output.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[2]->pan.ptr(), data_type == MASTER  );
                for( int input_id = 0 ; input_id != SUM_INPUTS_PER_FILTER ; ++input_id )
                {
                    morph_group_2->register_parameter( filter_datas[2]->input_envs[input_id]->attack.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[2]->input_envs[input_id]->decay.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[2]->input_envs[input_id]->sustain.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[2]->input_envs[input_id]->sustain_time.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[2]->input_envs[input_id]->release.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[2]->input_envs[input_id]->shape.ptr(), data_type == MASTER  );
                    morph_group_2->register_parameter( filter_datas[2]->input_sustains[input_id].ptr(), data_type == MASTER  );
                }

                morph_group_2->register_switch_parameter( filter_datas[2]->filter_type.int_ptr(), data_type == MASTER  );
                morph_group_2->register_switch_parameter( filter_datas[2]->modulate_distortion.bool_ptr(), data_type == MASTER  );
                morph_group_2->register_switch_parameter( filter_datas[2]->modulate_cutoff.bool_ptr(), data_type == MASTER  );
                morph_group_2->register_switch_parameter( filter_datas[2]->modulate_resonance.bool_ptr(), data_type == MASTER  );
                for( int input_id = 0 ; input_id != SUM_INPUTS_PER_FILTER ; ++input_id )
                {
                    morph_group_2->register_switch_parameter( reinterpret_cast< BoolParameter* >( filter_datas[2]->input_holds[input_id].ptr() ), data_type == MASTER  );
                }
                morph_group_2->register_switch_parameter( filter_datas[2]->modulate_output.bool_ptr(), data_type == MASTER  );

                // LFO
                morph_group_2->register_parameter( lfo_datas[2]->speed.ptr(), data_type == MASTER  );

                // ENV
                morph_group_2->register_parameter( filter_datas[2]->env_data->attack.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[2]->env_data->decay.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[2]->env_data->sustain.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[2]->env_data->sustain_time.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[2]->env_data->release.ptr(), data_type == MASTER  );
                morph_group_2->register_parameter( filter_datas[2]->env_data->shape.ptr(), data_type == MASTER  );
            }
        }

        // MAIN
        {
            morph_group_3->register_parameter( volume.ptr(), data_type == MASTER );
            morph_group_3->register_parameter( env_data->attack.ptr(), data_type == MASTER  );
            morph_group_3->register_parameter( env_data->decay.ptr(), data_type == MASTER  );
            morph_group_3->register_parameter( env_data->sustain.ptr(), data_type == MASTER  );
            morph_group_3->register_parameter( env_data->sustain_time.ptr(), data_type == MASTER  );
            morph_group_3->register_parameter( env_data->release.ptr(), data_type == MASTER  );

            //speed_multi
        }

        // EQ
        {
            for( int band_id = 0 ; band_id != SUM_EQ_BANDS ; ++band_id )
            {
                morph_group_3->register_parameter( eq_data->velocity[band_id].ptr(), data_type == MASTER  );
                morph_group_3->register_parameter( eq_data->envs[band_id]->attack.ptr(), data_type == MASTER  );
                morph_group_3->register_parameter( eq_data->envs[band_id]->decay.ptr(), data_type == MASTER  );
                morph_group_3->register_parameter( eq_data->envs[band_id]->sustain.ptr(), data_type == MASTER  );
                morph_group_3->register_parameter( eq_data->envs[band_id]->sustain_time.ptr(), data_type == MASTER  );
                morph_group_3->register_parameter( eq_data->envs[band_id]->release.ptr(), data_type == MASTER  );
                morph_group_3->register_parameter( eq_data->envs[band_id]->shape.ptr(), data_type == MASTER  );

                morph_group_3->register_switch_parameter( eq_data->hold[band_id].bool_ptr(), data_type == MASTER  );
            }

            morph_group_3->register_parameter( eq_data->bypass.ptr(), data_type == MASTER  );
        }

        // FX
        {
            // MAIN
            morph_group_3->register_parameter( distortion.ptr(), data_type == MASTER );
            morph_group_3->register_parameter( shape.ptr(), data_type == MASTER );
            morph_group_3->register_parameter( effect_bypass.ptr(), data_type == MASTER  );
            // REVERB
            morph_group_3->register_parameter( reverb_data->room.ptr(), data_type == MASTER  );
            morph_group_3->register_parameter( reverb_data->dry_wet_mix.ptr(), data_type == MASTER  );
            morph_group_3->register_parameter( reverb_data->width.ptr(), data_type == MASTER  );
            morph_group_3->register_parameter( reverb_data->pan.ptr(), data_type == MASTER  );
            // DELAY
            morph_group_3->register_parameter( delay.ptr(), data_type == MASTER  );
            morph_group_3->register_parameter( delay_pan.ptr(), data_type == MASTER  );
            // CHORUS
            morph_group_3->register_parameter( chorus_data->modulation.ptr(), data_type == MASTER  );
            morph_group_3->register_parameter( chorus_data->pan.ptr(), data_type == MASTER  );
        }

        // ARP
        {
            {
                for( int step_id = 0 ; step_id != SUM_ENV_ARP_STEPS ; ++step_id )
                {
                    morph_group_4->register_parameter( arp_sequencer_data->tune[step_id].ptr(), data_type == MASTER  );
                }
            }

            {
                for( int step_id = 0 ; step_id != SUM_ENV_ARP_STEPS ; ++step_id )
                {
                    morph_group_4->register_parameter( arp_sequencer_data->velocity[step_id].ptr(), data_type == MASTER  );
                }
            }

            {
                morph_group_4->register_parameter( arp_sequencer_data->shuffle.ptr(), data_type == MASTER  );
                morph_group_4->register_parameter( glide.ptr(), data_type == MASTER  );
            }

            {
                // is_on
                // speed_multi
                morph_group_4->register_switch_parameter( arp_sequencer_data->connect.bool_ptr(), data_type == MASTER  );
                //morph_group_arp_switchs->register_switch_parameter( arp_sequencer_data.connect.ptr(), data_type == MASTER  );
                for( int step_id = 0 ; step_id != SUM_ENV_ARP_STEPS ; ++step_id )
                {
                    morph_group_4->register_switch_parameter( arp_sequencer_data->step[step_id].bool_ptr(), data_type == MASTER  );
                }
            }

            {
                morph_group_4->register_parameter( velocity_glide_time.ptr(), data_type == MASTER  );
            }
        }
    }

    // MAKE IT HOT
    // ONLY THE MASTER HAS MORPHE SORCES - OTHERWISE WE BUILD UNLIMITED SOURCES FOR SOURCE
    if( data_type == MASTER )
    {
        left_morph_sources.add( new MoniqueSynthData(static_cast< DATA_TYPES >( MORPH ), nullptr, nullptr, nullptr, nullptr, nullptr, smooth_manager) );
        right_morph_sources.add( new MoniqueSynthData(static_cast< DATA_TYPES >( MORPH ), nullptr, nullptr, nullptr, nullptr, nullptr, smooth_manager ) );
        left_morph_sources.add( new MoniqueSynthData(static_cast< DATA_TYPES >( MORPH ), nullptr, nullptr, nullptr, nullptr, nullptr, smooth_manager ) );
        right_morph_sources.add( new MoniqueSynthData(static_cast< DATA_TYPES >( MORPH ), nullptr, nullptr, nullptr, nullptr, nullptr, smooth_manager ) );
        left_morph_sources.add( new MoniqueSynthData(static_cast< DATA_TYPES >( MORPH ), nullptr, nullptr, nullptr, nullptr, nullptr, smooth_manager ) );
        right_morph_sources.add( new MoniqueSynthData(static_cast< DATA_TYPES >( MORPH ), nullptr, nullptr, nullptr, nullptr, nullptr, smooth_manager ) );
        left_morph_sources.add( new MoniqueSynthData(static_cast< DATA_TYPES >( MORPH ), nullptr, nullptr, nullptr, nullptr, nullptr, smooth_manager ) );
        right_morph_sources.add( new MoniqueSynthData(static_cast< DATA_TYPES >( MORPH ), nullptr, nullptr, nullptr, nullptr, nullptr, smooth_manager ) );

        // SETUP THE MORPH GROUP
        // TODO, do not initalize the unneded morph groups
        // TODO, only load and save the needed params
        morph_group_1->set_sources( left_morph_sources[0]->morph_group_1, right_morph_sources[0]->morph_group_1, morhp_states[0], morhp_switch_states[0] );
        morph_group_2->set_sources( left_morph_sources[1]->morph_group_2, right_morph_sources[1]->morph_group_2, morhp_states[1], morhp_switch_states[1] );
        morph_group_3->set_sources( left_morph_sources[2]->morph_group_3, right_morph_sources[2]->morph_group_3, morhp_states[2], morhp_switch_states[2] );
        morph_group_4->set_sources( left_morph_sources[3]->morph_group_4, right_morph_sources[3]->morph_group_4, morhp_states[3], morhp_switch_states[3] );

        for( int morpher_id = 0 ; morpher_id != SUM_MORPHER_GROUPS ; ++morpher_id )
        {
            morhp_states[morpher_id].register_listener(this);
        }
        linear_morhp_state.register_listener(this);
    }
    left_morph_sources.minimiseStorageOverheads();
    right_morph_sources.minimiseStorageOverheads();
}
const String& MoniqueSynthData::get_morph_source_name( int id_abs_ ) const noexcept
{
    // LEFT
    if( id_abs_ < 4 )
    {
        return right_morph_source_names[id_abs_];
    }
    // RIGHT
    else
    {
        return left_morph_source_names[id_abs_-4];
    }
}
float MoniqueSynthData::get_morph_state( int morpher_id_ ) const noexcept
{
    return morhp_states[morpher_id_];
}
bool MoniqueSynthData::get_morph_switch_state( int morpher_id_ ) const noexcept
{
    return morhp_switch_states[morpher_id_];
}

//==============================================================================
void MoniqueSynthData::morph( int morpher_id_, float morph_amount_left_to_right_, bool force_ ) noexcept
{
    ScopedLock locked( morph_lock );

    if( force_ )
    {
        morhp_states[morpher_id_].get_runtime_info().stop_time_change();
        morhp_states[morpher_id_] = morph_amount_left_to_right_;
    }

    switch( morpher_id_ )
    {
    case 0 :
        morph_group_1->morph( morph_amount_left_to_right_ );
        break;
    case 1 :
        morph_group_2->morph( morph_amount_left_to_right_ );
        break;
    case 2 :
        morph_group_3->morph( morph_amount_left_to_right_ );
        break;
    case 3 :
        morph_group_4->morph( morph_amount_left_to_right_ );
        break;
    }
}
void MoniqueSynthData::morph_switch_buttons( int morpher_id_, bool do_switch_ ) noexcept
{
    ScopedLock locked( morph_lock );

    if( do_switch_ )
    {
        morhp_switch_states[morpher_id_] ^= true;
    }

    switch( morpher_id_ )
    {
    case 0 :
        morph_group_1->morph_switchs( morhp_switch_states[0] );
        break;
    case 1 :
        morph_group_2->morph_switchs( morhp_switch_states[1] );
        break;
    case 2 :
        morph_group_3->morph_switchs( morhp_switch_states[2] );
        break;
    case 3 :
        morph_group_4->morph_switchs( morhp_switch_states[3] );
        break;
    }
}
void MoniqueSynthData::run_sync_morph() noexcept
{
    morph_group_1->run_sync_morph();
    morph_group_2->run_sync_morph();
    morph_group_3->run_sync_morph();
    morph_group_4->run_sync_morph();
}

//==============================================================================
void MoniqueSynthData::parameter_value_changed( Parameter* param_ ) noexcept
{
    param_->get_runtime_info().stop_time_change();
    parameter_value_changed_by_automation( param_ );
}
void MoniqueSynthData::parameter_value_changed_by_automation( Parameter* param_ ) noexcept
{
    if( param_ == morhp_states[0].ptr() )
    {
        morph( 0, *param_ );
    }
    else if( param_ == morhp_states[1].ptr() )
    {
        morph( 1, *param_ );
    }
    else if( param_ == morhp_states[2].ptr() )
    {
        morph( 2, *param_ );
    }
    else if( param_ == morhp_states[3].ptr() )
    {
        morph( 3, *param_ );
    }
    else if( param_ == linear_morhp_state.ptr() )
    {
        float value = *param_;
        if( value <= 1 )
        {
            ChangeParamOverTime::execute( morhp_states[0], 1.0f-value, morph_motor_time );
            ChangeParamOverTime::execute( morhp_states[1], value, morph_motor_time );

            // SMOTH TO ZERO
            float morph_state_2 = get_morph_state(2);
            if( morph_state_2 != 0 )
            {
                ChangeParamOverTime::execute( morhp_states[2], 0, morph_motor_time );
            }
            float morph_state_3 = get_morph_state(3);
            if( morph_state_3 != 0 )
            {
                ChangeParamOverTime::execute( morhp_states[3], 0, morph_motor_time );
            }
        }
        else if( value <= 2 )
        {
            value -= 1;

            ChangeParamOverTime::execute( morhp_states[1], 1.0f-value, morph_motor_time );
            ChangeParamOverTime::execute( morhp_states[2], value, morph_motor_time );

            // SMOTH TO ZERO
            float morph_state_0 = get_morph_state(0);
            if( morph_state_0 != 0 )
            {
                ChangeParamOverTime::execute( morhp_states[0], 0, morph_motor_time );
            }
            float morph_state_3 = get_morph_state(3);
            if( morph_state_3 != 0 )
            {
                ChangeParamOverTime::execute( morhp_states[3], 0, morph_motor_time );
            }
        }
        else
        {
            value -= 2;

            ChangeParamOverTime::execute( morhp_states[2], 1.0f-value, morph_motor_time );
            ChangeParamOverTime::execute( morhp_states[3], value, morph_motor_time );

            // SMOTH TO ZERO
            float morph_state_0 = get_morph_state(0);
            if( morph_state_0 != 0 )
            {
                ChangeParamOverTime::execute( morhp_states[0], 0, morph_motor_time );
            }
            float morph_state_1 = get_morph_state(1);
            if( morph_state_1 != 0 )
            {
                ChangeParamOverTime::execute( morhp_states[1], 0, morph_motor_time );
            }
        }
    }
}

//==============================================================================
void MoniqueSynthData::set_morph_source_data_from_current( int morpher_id_, bool left_or_right_, bool run_sync_morph_ ) noexcept
{
    MorphGroup* morph_group_to_update;
    MorphGroup* morph_group_source;
    switch( morpher_id_ )
    {
    case 0 :
    {
        if( left_or_right_ == LEFT )
        {
            morph_group_to_update = left_morph_sources[0]->morph_group_1;
            left_morph_source_names.getReference(0) = "UPDATED FROM CURRENT";
        }
        else
        {
            morph_group_to_update = right_morph_sources[0]->morph_group_1;
            right_morph_source_names.getReference(0) = "UPDATED FROM CURRENT";
        }

        morph_group_source = morph_group_1;
        break;
    }
    case 1 :
    {
        if( left_or_right_ == LEFT )
        {
            morph_group_to_update = left_morph_sources[1]->morph_group_2;
            left_morph_source_names.getReference(1) = "UPDATED FROM CURRENT";
        }
        else
        {
            morph_group_to_update = right_morph_sources[1]->morph_group_2;
            right_morph_source_names.getReference(1) = "UPDATED FROM CURRENT";
        }

        morph_group_source = morph_group_2;
        break;
    }
    case 2 :
    {
        if( left_or_right_ == LEFT )
        {
            morph_group_to_update = left_morph_sources[2]->morph_group_3;
            left_morph_source_names.getReference(2) = "UPDATED FROM CURRENT";
        }
        else
        {
            morph_group_to_update = right_morph_sources[2]->morph_group_3;
            right_morph_source_names.getReference(2) = "UPDATED FROM CURRENT";
        }

        morph_group_source = morph_group_3;
        break;
    }
    case 3 :
    {
        if( left_or_right_ == LEFT )
        {
            morph_group_to_update = left_morph_sources[3]->morph_group_4;
            left_morph_source_names.getReference(3) = "UPDATED FROM CURRENT";
        }
        else
        {
            morph_group_to_update = right_morph_sources[3]->morph_group_4;
            right_morph_source_names.getReference(3) = "UPDATED FROM CURRENT";
        }

        morph_group_source = morph_group_4;
        break;
    }
    }

    for( int i = 0 ; i != morph_group_to_update->params.size() ; ++i )
    {
        Parameter*param( morph_group_to_update->params.getUnchecked(i) );
        Parameter*source_param( morph_group_source->params.getUnchecked(i) );
        param->set_value_without_notification( source_param->get_value() );
        param->set_modulation_amount_without_notification( source_param->get_modulation_amount() );
    }

    if( run_sync_morph_ )
    {
        run_sync_morph();
    }
}

void MoniqueSynthData::refresh_morph_programms() noexcept
{
    left_morph_sources.getUnchecked( 0 )->banks = banks;
    left_morph_sources.getUnchecked( 1 )->banks = banks;
    left_morph_sources.getUnchecked( 2 )->banks = banks;
    left_morph_sources.getUnchecked( 3 )->banks = banks;
    right_morph_sources.getUnchecked( 0 )->banks = banks;
    right_morph_sources.getUnchecked( 1 )->banks = banks;
    right_morph_sources.getUnchecked( 2 )->banks = banks;
    right_morph_sources.getUnchecked( 3 )->banks = banks;
    left_morph_sources.getUnchecked( 0 )->program_names_per_bank = program_names_per_bank;
    left_morph_sources.getUnchecked( 1 )->program_names_per_bank = program_names_per_bank;
    left_morph_sources.getUnchecked( 2 )->program_names_per_bank = program_names_per_bank;
    left_morph_sources.getUnchecked( 3 )->program_names_per_bank = program_names_per_bank;
    right_morph_sources.getUnchecked( 0 )->program_names_per_bank = program_names_per_bank;
    right_morph_sources.getUnchecked( 1 )->program_names_per_bank = program_names_per_bank;
    right_morph_sources.getUnchecked( 2 )->program_names_per_bank = program_names_per_bank;
    right_morph_sources.getUnchecked( 3 )->program_names_per_bank = program_names_per_bank;
}

bool MoniqueSynthData::try_to_load_programm_to_left_side( int morpher_id_, int bank_id_, int index_ ) noexcept
{
    MoniqueSynthData* synth_data = left_morph_sources.getUnchecked( morpher_id_ );
    synth_data->set_current_bank( bank_id_ );
    synth_data->set_current_program( index_ );
    bool success = synth_data->load( );
    if( success )
    {
        run_sync_morph();
        left_morph_source_names.getReference(morpher_id_) = synth_data->get_current_program_name_abs();
    }

    return success;
}
bool MoniqueSynthData::try_to_load_programm_to_right_side( int morpher_id_, int bank_id_, int index_ ) noexcept
{
    MoniqueSynthData* synth_data = right_morph_sources.getUnchecked( morpher_id_ );
    synth_data->set_current_bank( bank_id_ );
    synth_data->set_current_program( index_ );
    bool success = synth_data->load( );
    if( success )
    {
        run_sync_morph();
        right_morph_source_names.getReference(morpher_id_) = synth_data->get_current_program_name_abs();
    }

    return success;
}

//==============================================================================
//==============================================================================
//==============================================================================
void MoniqueSynthData::refresh_banks_and_programms( MoniqueSynthData& synth_data ) noexcept
{
    // BANKS
    synth_data.banks.clearQuick();
    update_banks( synth_data.banks );

    // PROGRAMMS PER BANK
    synth_data.program_names_per_bank.clearQuick();
    for( int i = 0 ; i != 26 ; ++i )
    {
        synth_data.program_names_per_bank.add( StringArray() );
    }
    for( int i = 0 ; i != 26 ; ++i )
    {
        update_bank_programms( synth_data, i, synth_data.program_names_per_bank.getReference(i) );
    }

    synth_data.calc_current_program_abs();
    synth_data.refresh_morph_programms();
}
void MoniqueSynthData::calc_current_program_abs() noexcept
{
    if( current_program == -1 )
    {
        current_program_abs = -1;
        return;
    }

    current_program_abs = 0;
    for( int bank_id = 0 ; bank_id != current_bank ; ++bank_id )
    {
        int bank_size = program_names_per_bank.getReference(bank_id).size();
        if( current_program_abs+current_program < bank_size )
        {
            current_program_abs += current_program;
            break;
        }
        else
        {
            current_program_abs += bank_size;
        }
    }
}
void MoniqueSynthData::update_banks( StringArray& banks_ ) noexcept
{
    banks_.add("A");
    banks_.add("B");
    banks_.add("C");
    banks_.add("D");
    banks_.add("E");
    banks_.add("F");
    banks_.add("G");
    banks_.add("H");
    banks_.add("I");
    banks_.add("J");
    banks_.add("K");
    banks_.add("L");
    banks_.add("M");
    banks_.add("N");
    banks_.add("O");
    banks_.add("P");
    banks_.add("Q");
    banks_.add("R");
    banks_.add("S");
    banks_.add("T");
    banks_.add("U");
    banks_.add("V");
    banks_.add("W");
    banks_.add("X");
    banks_.add("Y");
    banks_.add("Z");
}
static inline File get_bank_folder( const String& bank_name_ ) noexcept
{
    File folder = File::getSpecialLocation(File::SpecialLocationType::ROOT_FOLDER);
    folder = File(folder.getFullPathName()+PROJECT_FOLDER+bank_name_);
    folder.createDirectory();

    return folder;
}
static inline void sort_by_name( Array< File >& file_array_ ) noexcept
{
    for( int file_to_sort_id = 0; file_to_sort_id != file_array_.size() ; ++file_to_sort_id )
    {
        String to_move_name = file_array_.getReference(file_to_sort_id).getFileName();
        for( int compare_id = 0 ; compare_id != file_array_.size() ; ++compare_id )
        {
            if( file_to_sort_id != compare_id )
            {
                if( file_array_.getReference(compare_id).getFileName().compareNatural(to_move_name) > 0 )
                {
                    file_array_.swap( file_to_sort_id, compare_id );

                    // RESET FIRST LOOP
                    file_to_sort_id--; // -1 for ++LOOP
                    break;
                }
            }
        }
    }
}
/*
static inline void sort_by_date( Array< File >& file_array_ )
{
    for( int file_to_sort_id = 0; file_to_sort_id != file_array_.size() ; ++file_to_sort_id )
    {
        Time to_move_time = file_array_.getReference(file_to_sort_id).getLastModificationTime();
        for( int compare_id = 0 ; compare_id != file_array_.size() ; ++compare_id )
        {
            if( file_to_sort_id != compare_id )
            {
                Time to_compare_time = file_array_.getReference(compare_id).getLastModificationTime();
                if( to_compare_time < to_move_time )
                {
                    file_array_.swap( file_to_sort_id, compare_id );

                    // RESET FIRST LOOP
                    file_to_sort_id--; // -1 for ++LOOP
                    break;
                }
            }
        }
    }
}
*/
void MoniqueSynthData::update_bank_programms( MoniqueSynthData& synth_data, int bank_id_, StringArray& program_names_ ) noexcept
{
    File bank_folder = get_bank_folder( synth_data.banks[bank_id_] );
    Array< File > program_files;
    bank_folder.findChildFiles( program_files, File::findFiles, false, "*.mlprog" );
    //sort_by_date(program_files);
    sort_by_name(program_files);

    for( int i = 0 ; i != program_files.size() ; ++i )
    {
        program_names_.add( program_files.getReference(i).getFileNameWithoutExtension() );
    }
}

//==============================================================================
const StringArray& MoniqueSynthData::get_banks() noexcept
{
    return banks;
}
const StringArray& MoniqueSynthData::get_programms( int bank_id_ ) noexcept
{
    return program_names_per_bank.getReference(bank_id_);
}

// ==============================================================================
void MoniqueSynthData::set_current_bank( int bank_index_ ) noexcept
{
    if( current_bank != bank_index_ )
    {
        current_bank = bank_index_;
        current_program = -1; // TODO can be an empty bank

        calc_current_program_abs();
    }
}
void MoniqueSynthData::set_current_program( int programm_index_ ) noexcept
{
    if( current_program != programm_index_ )
    {
        current_program = programm_index_< program_names_per_bank.getReference(current_bank).size() ? programm_index_ : current_program;
        if( current_program == programm_index_ )
        {
            calc_current_program_abs();
        }
    }
}
void MoniqueSynthData::set_current_program_abs( int programm_index_ ) noexcept
{
    int sum_programms = 0;

    for( int bank_id = 0 ; bank_id != banks.size() ; ++bank_id )
    {
        int bank_size = program_names_per_bank.getReference(bank_id).size();
        if( programm_index_ < bank_size+sum_programms )
        {
            current_bank = bank_id;
            current_program = programm_index_-sum_programms;
            current_program_abs = programm_index_;
            break;
        }
        sum_programms+=bank_size;
    }
}

// ==============================================================================
int MoniqueSynthData::get_current_bank() const noexcept
{
    return current_bank;
}
int MoniqueSynthData::get_current_program() const noexcept
{
    return current_program;
}
const StringArray& MoniqueSynthData::get_current_bank_programms() const noexcept
{
    return program_names_per_bank.getReference(current_bank);
}

// ==============================================================================
int MoniqueSynthData::get_current_programm_id_abs() const noexcept
{
    return current_program_abs;
}
const String& MoniqueSynthData::get_current_program_name_abs() const noexcept
{
    if( current_program == -1 )
    {
        return error_string;
    }
    return program_names_per_bank.getReference(current_bank)[current_program];
}
const String& MoniqueSynthData::get_program_name_abs(int id_) const noexcept
{
    for( int bank_id = 0 ; bank_id != banks.size() ; ++bank_id )
    {
        const int bank_size = program_names_per_bank.getReference(bank_id).size();
        if( id_ < bank_size )
        {
            return program_names_per_bank.getReference(bank_id)[id_];
        }
        else
        {
            id_ -= bank_size;
        }
    }

    return error_string;
}

// ==============================================================================
static inline File get_program_file( const String& bank_name_, const String& program_name_ ) noexcept
{
    return File( get_bank_folder( bank_name_ ).getFullPathName()
    + String("/")
    + program_name_
    + ".mlprog");
}
static inline String& generate_programm_name( const String& bank_, String& name_ ) noexcept
{
    bool exist = false;
    int counter = 1;
    String counter_name("");
    do
    {
        File program = get_program_file( bank_, name_ + counter_name );
        if( program.exists() )
        {
            counter_name = String(" - ")+ String(counter);
            counter++;
            exist = true;
        }
        else
        {
            name_ = name_+counter_name;
            exist = false;
        }
    } while( exist );

    return name_;
}
void MoniqueSynthData::create_internal_backup( const String& programm_name_, const String& bank_name_ ) noexcept
{
    last_bank = bank_name_;
    last_program = programm_name_;

    saveable_backups.clearQuick();
    for( int i = 0 ; i != saveable_parameters.size() ; ++i )
    {
        saveable_backups.add( saveable_parameters.getUnchecked(i)->get_value() );
    }


    alternative_program_name = last_program;
}
bool MoniqueSynthData::create_new( const String& new_name_ ) noexcept
{
    String name_to_use = new_name_;
    generate_programm_name( banks[current_bank], name_to_use );

    bool success = write2file( banks[current_bank], name_to_use );

    if( success )
    {
        refresh_banks_and_programms( *this );
        current_program = program_names_per_bank.getReference(current_bank).indexOf(name_to_use);

        create_internal_backup( new_name_, banks[current_bank] );
    }

    return success;
}
bool MoniqueSynthData::rename( const String& new_name_ ) noexcept
{
    if( current_program == -1 )
        return false;

    File program = get_program_file( banks[current_bank], program_names_per_bank.getReference(current_bank)[current_program] );

    String name = new_name_;
    bool success = false;
    generate_programm_name( banks[current_bank], name );
    if( program.existsAsFile() )
    {
        success = program.moveFileTo
        (
            get_bank_folder(banks[current_bank]).getFullPathName()
            + String("/")
            + name
            + ".mlprog"
        );
    }

    if( success )
    {
        refresh_banks_and_programms( *this );
        current_program = program_names_per_bank.getReference(current_bank).indexOf(new_name_);

        create_internal_backup( new_name_, banks[current_bank] );
    }

    return success;
}
bool MoniqueSynthData::replace() noexcept
{
    if( current_program == -1 )
        return false;

    String bank_name = banks[current_bank];
    String program_name = program_names_per_bank.getReference(current_bank)[current_program];
    File program = get_program_file( bank_name, program_name );
    bool success = true;
    /*
    AlertWindow::showOkCancelBox
    (
        AlertWindow::AlertIconType::QuestionIcon,
        "REPLACE PROJECT?",
        String("Overwrite project: ")+bank_name+String(":")+program_name,
        "YES", "NO"
    );
    */
    if( success )
    {
        success = write2file( bank_name, program_name );
        if( success )
        {
            create_internal_backup( program_name, bank_name );
        }
    }

    return success;
}
bool MoniqueSynthData::remove() noexcept
{
    if( current_program == -1 )
        return false;

    String old_program_name = program_names_per_bank.getReference(current_bank)[current_program];
    String old_bank_name = banks[current_bank];
    File program = get_program_file( old_bank_name, old_program_name );
    bool success = AlertWindow::showOkCancelBox
    (
        AlertWindow::AlertIconType::QuestionIcon,
        "DELETE PROJECT?",
        String("Delete project: ")+old_bank_name+String(":")+old_program_name,
        "YES", "NO"
    );
    if( success )
    {
        program.moveToTrash();
        current_program = -1;
        refresh_banks_and_programms( *this );

        create_internal_backup( String("REMOVED: ")+old_program_name, old_bank_name );
    }

    return success;
}

// ==============================================================================
bool MoniqueSynthData::load( bool load_morph_groups, bool ignore_warnings_ ) noexcept
{
    if( current_program == -1 )
        return false;

    return load
    (
        banks[current_bank],
        program_names_per_bank.getReference(current_bank)[current_program],
        load_morph_groups,
        ignore_warnings_
    );
}
bool MoniqueSynthData::load_prev() noexcept
{
    bool success = false;

    if( current_program-1 >= 0 )
    {
        current_program--;
        success = load();
    }
    else
    {
        int last_index = program_names_per_bank.getReference(current_bank).size()-1;
        if( last_index > 0 )
        {
            current_program = last_index;
            success = load();
        }
    }

    return success;
}
bool MoniqueSynthData::load_next() noexcept
{
    bool success = false;

    if( current_program+1 < program_names_per_bank.getReference(current_bank).size() )
    {
        current_program++;
        success = load();
    }
    else
    {
        if( program_names_per_bank.getReference(current_bank).size() )
        {
            current_program = 0;
            success = load();
        }
    }

    return success;
}
#include "monique_ui_MainWindow.h"
bool MoniqueSynthData::load( const String bank_name_, const String program_name_, bool load_morph_groups, bool ignore_warnings_ ) noexcept
{
    bool success = false;
    File program_file = get_program_file( bank_name_, program_name_ );
    // last_bank = bank_name_;
    // last_program = program_name_;
    ScopedPointer<XmlElement> xml = XmlDocument( program_file ).getDocumentElement();
    if( xml )
    {
        if( xml->hasTagName("PROJECT-1.0") || xml->hasTagName("MONOLisa") )
        {
            read_from(xml);
            success = true;

            /* NOTE OPTION
                if( Monique_Ui_Mainwindow*mainwindow = AppInstanceStore::getInstance()->editor )
                {
                    mainwindow->update_slider_return_values();
                }
                */
        }
        else
        {
            success = false;
        }
    }

    return success;
}

// ==============================================================================
void MoniqueSynthData::load_default() noexcept
{
    ScopedPointer<XmlElement> xml = XmlDocument::parse( BinaryData::FACTORTY_DEFAULT_mlprog );
    read_from( xml );
    alternative_program_name = FACTORY_NAME;
    //current_bank = 0;
    current_program = -1;
}
// ==============================================================================
void MoniqueSynthData::save_to( XmlElement* xml_ ) noexcept
{
    if( xml_ )
    {
        // REPLACE ARP OTIONS
        {
            if( keep_arp_always_on )
            {
                arp_sequencer_data->is_on = true;
            }
            if( keep_arp_always_off )
            {
                arp_sequencer_data->is_on = false;
            }
        }

        for( int i = 0 ; i != saveable_parameters.size() ; ++i )
        {
            write_parameter_to_file( *xml_, saveable_parameters.getUnchecked(i) );
        }

        if( id == MASTER )
        {
            for( int morpher_id = 0 ; morpher_id != SUM_MORPHER_GROUPS ; ++morpher_id )
            {
                xml_->setAttribute( String("left_morph_source_")+String( morpher_id ), left_morph_source_names[morpher_id] );
                left_morph_sources[morpher_id]->save_to(xml_->createNewChildElement(String("LeftMorphData_")+String(morpher_id)));
                xml_->setAttribute( String("right_morph_source_")+String( morpher_id ), right_morph_source_names[morpher_id] );
                right_morph_sources[morpher_id]->save_to(xml_->createNewChildElement(String("RightMorphData_")+String(morpher_id)));
            }

            create_internal_backup( program_names_per_bank.getReference(current_bank)[current_program], banks[current_bank] );
        }
    }
}
bool MoniqueSynthData::write2file( const String& bank_name_, const String& program_name_ ) noexcept
{
    File program_file = get_program_file( bank_name_, program_name_ );

    XmlElement xml("PROJECT-1.0");
    save_to( &xml );

    return xml.writeToFile(program_file,"");
}
void MoniqueSynthData::read_from( const XmlElement* xml_ ) noexcept
{
    if( xml_ )
    {
        // PARAMS
        // TODO, this is not really required. coz the morph groubs will set this values
        //if( id != MASTER )
        {
            for( int i = 0 ; i != saveable_parameters.size() ; ++i )
            {
                Parameter*const param = saveable_parameters.getUnchecked(i);
                read_parameter_from_file( *xml_, saveable_parameters.getUnchecked(i) );
                /*
		if( param->get_info().name.contains("max_release") )
                {
                    Parameter*const param_before = saveable_parameters.getUnchecked(i-1);
                    param_before->set_value( jmin(1.0f,(1.0f /5000.0f*param_before->get_value()*param->get_value())) );
                }
                if( param->get_info().name.contains("max_decay") )
                {
                    Parameter*const param_before = saveable_parameters.getUnchecked(i-1);
                    param_before->set_value( jmin(1.0f,(1.0f /5000.0f*param_before->get_value()*param->get_value())) ) ;
                }
                if( param->get_info().name.contains("max_attack") )
                {
                    Parameter*const param_before = saveable_parameters.getUnchecked(i-1);
                    param_before->set_value( jmin(1.0f,(1.0f /5000.0f*param_before->get_value()*param->get_value())) ) ;
                }
                */
            }
        }


        // MORPH STUFF
        if( id == MASTER )
        {
            for( int morpher_id = 0 ; morpher_id != SUM_MORPHER_GROUPS ; ++morpher_id )
            {
                left_morph_source_names.getReference(morpher_id) = xml_->getStringAttribute( String("left_morph_source_")+String( morpher_id ), "FACTORY DEFAULT" );
                left_morph_sources[morpher_id]->read_from(xml_->getChildByName(String("LeftMorphData_")+String(morpher_id)));
                right_morph_source_names.getReference(morpher_id) = xml_->getStringAttribute( String("right_morph_source_")+String( morpher_id ), "FACTORY DEFAULT" );
                right_morph_sources[morpher_id]->read_from(xml_->getChildByName(String("RightMorphData_")+String(morpher_id)));
            }

            for( int morpher_id = 0 ; morpher_id != SUM_MORPHER_GROUPS ; ++morpher_id )
            {
                morph( morpher_id, morhp_states[morpher_id] );
                morph_switch_buttons( morpher_id, false );
            }

            create_internal_backup( program_names_per_bank.getReference(current_bank)[current_program], banks[current_bank] );
        }
    }
}
//==============================================================================
void MoniqueSynthData::save_settings() const noexcept
{
    File folder = File::getSpecialLocation(File::SpecialLocationType::ROOT_FOLDER);
    folder = File(folder.getFullPathName()+PROJECT_FOLDER);
    if( folder.createDirectory() )
    {
        File settings_session_file( File( folder.getFullPathName() + String("/") + "session.mcfg") );

        XmlElement xml("SETTINGS-1.0");
        for( int i = 0 ; i != global_parameters.size() ; ++i )
        {
            write_parameter_to_file( xml, global_parameters.getUnchecked(i) );
        }

#ifdef IS_STANDALONE
        xml.setAttribute( "BANK", current_bank );
        xml.setAttribute( "PROG", current_program );
#endif

        ui_look_and_feel->colours.save_to( &xml );

        xml.writeToFile(settings_session_file,"");
    }
}
void MoniqueSynthData::ask_and_save_if_changed( bool with_new_option ) noexcept
{
    // CHECK FOR CHANGES FIRST
    for( int i = 0 ; i != saveable_backups.size() ; ++i )
    {
        if( saveable_backups.getUnchecked(i) != saveable_parameters.getUnchecked(i)->get_value() )
        {
            const bool is_restored_programm = alternative_program_name.startsWith("0RIGINAL WAS: ");

            int success = 0;
            // - 0 if the third button was pressed (normally used for 'cancel')
            // - 1 if the first button was pressed (normally used for 'yes')
            // - 2 if the middle button was pressed (normally used for 'no')
            if( !is_restored_programm and alternative_program_name != FACTORY_NAME and current_program != -1 )
            {
                success = AlertWindow::showOkCancelBox
                (
                    AlertWindow::AlertIconType::QuestionIcon,
                    "CURRENT PROJECT CHANGED!",
                    String("Do you like to store your changes to '") + last_bank + String(":") + last_program + String( "' first?"),
                    "YES",
                    //"CREATE A BACKUP (_backup)",
                    "NO"
                );
            }

            if( success == 1 )
            {
                write2file( last_bank, last_program );
            }
            else if( success == 0 )
            {
            }
            else if( success == 2 )
            {
                create_new( last_program + String("_backup") );
            }

            break;
        }
    }
}
void MoniqueSynthData::load_settings() noexcept
{
    File folder = File::getSpecialLocation(File::SpecialLocationType::ROOT_FOLDER);
    File settings_session_file = File(folder.getFullPathName()+PROJECT_FOLDER+String("session.mcfg"));
    ScopedPointer<XmlElement> xml = XmlDocument( settings_session_file ).getDocumentElement();
    if( xml )
    {
        if( xml->hasTagName("SETTINGS-1.0") )
        {
            for( int i = 0 ; i != global_parameters.size() ; ++i )
            {
                read_parameter_from_file( *xml, global_parameters.getUnchecked(i) );
            }
        }
#ifdef IS_STANDALONE
        current_bank = xml->getIntAttribute( "BANK", 0 );
        current_program = xml->getIntAttribute( "PROG", -1 );
#endif

        ui_look_and_feel->colours.read_from( xml );
    }
}

//==============================================================================
void MoniqueSynthData::save_midi() const noexcept
{
    File folder = File::getSpecialLocation(File::SpecialLocationType::ROOT_FOLDER);
    folder = File(folder.getFullPathName()+PROJECT_FOLDER);
    if( folder.createDirectory() )
    {
        File midi_file( File( folder.getFullPathName() + String("/") + "patch.midi") );

        XmlElement xml("MIDI-PATCH-1.0");
        for( int i = 0 ; i != saveable_parameters.size() ; ++i )
        {
            write_midi_to( xml, saveable_parameters.getUnchecked(i) );
        }

        xml.writeToFile(midi_file,"");
    }
}
void MoniqueSynthData::read_midi() noexcept
{
    File folder = File::getSpecialLocation(File::SpecialLocationType::ROOT_FOLDER);
    File midi_file = File(folder.getFullPathName()+PROJECT_FOLDER+String("patch.midi"));
    ScopedPointer<XmlElement> xml = XmlDocument( midi_file ).getDocumentElement();
    if( xml )
    {
        if( xml->hasTagName("MIDI-PATCH-1.0") )
        {
            for( int i = 0 ; i != saveable_parameters.size() ; ++i )
            {
                read_midi_from( *xml, saveable_parameters.getUnchecked(i), audio_processor );
            }
        }
    }
}






