#include <iostream>
#include <list>
#include <math.h>
#include <mutex>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#ifndef HYDRA_RESOURCE_INCLUDE_H
#define HYDRA_RESOURCE_INCLUDE_H

// Class that represents a spectrum chunk. Free chunks have an ID equal to zero
class chunk
{
public:
  // Centre Frequency in Hz
  double d_centre_freq;
  // Bandwidth in Hz
  double d_bandwidth;
  // Service index
  unsigned int u_id;

  // Class constructor
  chunk(double d_cf,
        double d_bw,
        unsigned int id = 0)
  {
    d_centre_freq = d_cf;
    d_bandwidth = d_bw;
    u_id = id;
  }

  // Returns content of the class instance
  boost::property_tree::ptree
  output()
  {
    // Create a tree object
    boost::property_tree::ptree tree;
    // Insert the class parameters
    tree.put("id", this->u_id);
    tree.put("bandwidth", this->d_bandwidth);
    tree.put("centre_freq", this->d_centre_freq);
    // Return the tree object
    return tree;
  }
};

class rf_front_end
{
  // List of chunks
  std::list<chunk> resources;

public:
  // Class constructor
  rf_front_end(double d_cf,
               double d_bw,
               unsigned int u_id = 0);

  // Create new chunks
  int create_chunks(double d_centre_freq,
                    double d_bandwidth,
                    unsigned int u_id = 0);

  // List chunks
  boost::property_tree::ptree list_chunks();

  // Delete chunk from chunk list and resize/create free spaces
  void delete_chunks(const unsigned int &u_id);
};

class xvl_resource_manager
{
private:
   // List of RX chunks
   std::unique_ptr<rf_front_end> rx_resources;
   // List of TX chunks
   std::unique_ptr<rf_front_end> tx_resources;
   // Mutex to make its method thread safe
   std::mutex mtx;
   // Flags
   bool b_receiver;
   bool b_transmitter;

public:
   // Class constructor
   xvl_resource_manager();

   // Class destructor
   ~xvl_resource_manager(){};

   // Set the RX resources
   void set_rx_resources(double d_centre_freq,
                         double d_bandwidth);

   // Set the TX resources
   void set_tx_resources(double d_centre_freq,
                         double d_bandwidth);

   // Method to reserve RX resources to a service
   int reserve_rx_resources(unsigned int u_id,
                            double d_centre_freq,
                            double d_bandwidth);

   // Method to reserve TX resources to a service
   int reserve_tx_resources(unsigned int u_id,
                            double d_centre_freq,
                            double d_bandwidth);

   // Method to query the list of chunks
   boost::property_tree::ptree query_resources();

   // Method to free chunks of a service
   int free_resources(size_t u_id);
};

#endif
