/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.2.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_15EBFFC85DA080CA__
#define __JUCE_HEADER_15EBFFC85DA080CA__

//[Headers]     -- You can add your own extra header files here --
#include "App_h_includer.h"

#include "monique_core_Datastructures.h"

//==============================================================================
//==============================================================================
//==============================================================================
class EndlessBuffer : public RuntimeListener
{
protected:
    CriticalSection writer_lock;
    CriticalSection reader_lock;
    mono_AudioSampleBuffer<1> sample_buffer;

    int current_size;
    int reader_position;

public:
    inline void write( const float* samples_, int num_samples_ ) noexcept;

public:
    inline void read_lock() noexcept;
    inline float get_next_and_count( int& pos_ ) const noexcept;
    inline void read_unlock() noexcept;

private:
    COLD virtual void sample_rate_changed( double /* old_sr_ */ ) noexcept override;
    COLD virtual void block_size_changed() noexcept override;

public:
    COLD EndlessBuffer();
    COLD ~EndlessBuffer();
};
//==============================================================================
inline void EndlessBuffer::write( const float* samples_, int num_samples_ ) noexcept
{
    float*const tmp_sample_buffer = sample_buffer.getWritePointer(0);
    int tmp_position = reader_position;
    for( int sid = 0 ; sid != num_samples_ ; ++sid )
    {
        tmp_sample_buffer[tmp_position] = samples_[sid];

        tmp_position++;
        if( tmp_position >= current_size )
            tmp_position = 0;
    }

    reader_position = tmp_position;
}

//==============================================================================
//==============================================================================
//==============================================================================
class EndlessSwitchBuffer : public EndlessBuffer
{
    mono_AudioSampleBuffer<1> switch_buffer;

public:
    inline void write( const float* samples_, const float* switchs_, int num_samples_ ) noexcept;
    inline int get_new_reader_start_position( int samples_to_paint_ ) const noexcept;

private:
    COLD void sample_rate_changed( double /* old_sr_ */ ) noexcept override;
    COLD void block_size_changed() noexcept override;

public:
    COLD EndlessSwitchBuffer();
    COLD ~EndlessSwitchBuffer();
};

//==============================================================================
inline void EndlessSwitchBuffer::write( const float* samples_, const float* switchs_, int num_samples_ ) noexcept
{
    float*const tmp_sample_buffer = sample_buffer.getWritePointer();
    float*const tmp_switch_buffer = switch_buffer.getWritePointer();
    int tmp_position = reader_position;
    for( int sid = 0 ; sid != num_samples_ ; ++sid )
    {
        tmp_sample_buffer[tmp_position] = samples_[sid];
        tmp_switch_buffer[tmp_position] = switchs_[sid];

        tmp_position++;
        if( tmp_position >= current_size )
            tmp_position = 0;
    }

    reader_position = tmp_position;
}
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class Monique_Ui_AmpPainter  : public Component,
    public Timer,
    public SliderListener,
    public ButtonListener
{
public:
    //==============================================================================
    Monique_Ui_AmpPainter ();
    ~Monique_Ui_AmpPainter();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    const float original_w;
    const float original_h;

private:
    OwnedArray<EndlessBuffer> filter_values;
    OwnedArray<EndlessBuffer> filter_env_values;
    EndlessBuffer eq_values;
    EndlessBuffer values_env;
    EndlessBuffer values;
    OwnedArray<EndlessSwitchBuffer> osc_values;

    Array<EndlessBuffer*> buffers;

public:
    inline void add_filter( int id_, const float* values_, int num_samples_ ) noexcept;
    inline void add_filter_env( int id_, const float* values_, int num_samples_ ) noexcept;
    inline void add_eq( const float* values_, int num_samples_ ) noexcept;
    inline void add_out_env( const float* values_, int num_samples_ ) noexcept;
    inline void add_out( const float* values_, int num_samples_ ) noexcept;
    inline void add_osc( int id_, const float* values_, const float* is_switch_values, int num_samples_ ) noexcept;
private:
    inline void lock_for_reading() noexcept;
    inline void unlock_for_reading() noexcept;

private:
    void timerCallback() override;

    void refresh_buttons();
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Slider> sl_show_range;
    ScopedPointer<TextButton> osc_1;
    ScopedPointer<TextButton> osc_2;
    ScopedPointer<TextButton> osc_3;
    ScopedPointer<TextButton> eq;
    ScopedPointer<TextButton> out;
    ScopedPointer<TextButton> f_1;
    ScopedPointer<TextButton> f_2;
    ScopedPointer<TextButton> f_3;
    ScopedPointer<TextButton> f_env_1;
    ScopedPointer<TextButton> f_env_2;
    ScopedPointer<TextButton> f_env_3;
    ScopedPointer<TextButton> out_env;
    ScopedPointer<Component> drawing_area;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Monique_Ui_AmpPainter)
};

//[EndFile] You can add extra defines here...
inline void Monique_Ui_AmpPainter::add_filter_env(int id_, const float* values_, int num_samples_) noexcept
{
    //if( show_filter_env[id_] )
    {
        EndlessBuffer*const values = filter_env_values.getUnchecked(id_);
        values->write( values_, num_samples_ );
    }
}
inline void Monique_Ui_AmpPainter::add_filter(int id_, const float* values_, int num_samples_) noexcept
{
    //if( show_filter[id_] )
    {
        EndlessBuffer*const values = filter_values.getUnchecked(id_);
        values->write( values_, num_samples_ );
    }
}
inline void Monique_Ui_AmpPainter::add_eq( const float* values_, int num_samples_ ) noexcept
{
    //if( show_eq )
    {
        eq_values.write( values_, num_samples_ );
    }
}
inline void Monique_Ui_AmpPainter::add_out_env( const float* values_, int num_samples_ ) noexcept
{
    //if( show_out_env )
    {
        values_env.write( values_, num_samples_ );
    }
}
inline void Monique_Ui_AmpPainter::add_out( const float* values_, int num_samples_ ) noexcept
{
    //if( show_out )
    {
        values.write( values_, num_samples_ );
    }
}
inline void Monique_Ui_AmpPainter::add_osc( int id_, const float* values_, const float* is_switch_values, int num_samples_ ) noexcept
{
    //if( id_ == 0 || show_osc[id_] )
    {
        EndlessSwitchBuffer*const values = osc_values.getUnchecked(id_);
        values->write( values_, is_switch_values, num_samples_ );
    }
};
//[/EndFile]

#endif   // __JUCE_HEADER_15EBFFC85DA080CA__
