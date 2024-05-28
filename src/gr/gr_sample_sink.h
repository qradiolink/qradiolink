#ifndef GR_SAMPLE_SINK_H
#define GR_SAMPLE_SINK_H


#include <gnuradio/sync_block.h>
#include <gnuradio/sync_interpolator.h>
#include <gnuradio/io_signature.h>
#include <stdio.h>

class gr_sample_sink;
typedef boost::shared_ptr<gr_sample_sink> gr_sample_sink_sptr;

gr_sample_sink_sptr make_gr_sample_sink();

class gr_sample_sink : public gr::sync_block
{
public:
    gr_sample_sink();
    ~gr_sample_sink();
    int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    std::vector<gr_complex>* get_data();
    void flush();
    void set_sample_window(unsigned int size);
    void set_enabled(bool value);

private:
    unsigned int _offset;
    unsigned int _window_size;
    bool _finished;
    bool _enabled;
    std::vector<gr_complex> *_data;
    gr::thread::condition_variable _cond_wait;
    gr::thread::mutex _mutex;
    boost::mutex _boost_mutex;
};

#endif // GR_SAMPLE_SINK_H
