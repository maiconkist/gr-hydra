#include "hydra/hydra_resource.h"

#include <boost/format.hpp>

namespace hydra {

// Methods of the resource manager class
xvl_resource_manager::xvl_resource_manager()
{
  // Set initial flags. The user must set the RX/TX resources.
  b_receiver = false;
  b_transmitter = false;
}

void
xvl_resource_manager::set_rx_resources(double d_centre_freq,
                                       double d_bandwidth)
{
  // If already set the RX resources
  if (b_receiver)
    {
      std::cerr << "Already configure the RX resources!"  << std::endl;
      exit(60);
  }

  // Toggle the receiver flag
  b_receiver = true;

  // Initial chunk, with all the available resources
  rx_resources = rf_front_end(d_centre_freq, d_bandwidth);
}

void
xvl_resource_manager::set_tx_resources(double d_centre_freq,
                                       double d_bandwidth)
{
  // If already set the TX resources
  if (b_transmitter)
  {
    std::cerr << "Already configure the TX resources!"  << std::endl;
    exit(60);
  }

  // Toggle the transmitter flag
  b_transmitter = true;

  // Initial chunk, with all the available resources
  tx_resources = rf_front_end(d_centre_freq, d_bandwidth);
}

int
xvl_resource_manager::reserve_rx_resources(unsigned int u_id,
                                           double d_centre_freq,
                                           double d_bandwidth)
{
  // If the RX resources were not defined
  if (not b_receiver)
  {
    std::cerr << __PRETTY_FUNCTION__ << ": RX Resources not configured." << std::endl;
    return 1;
  }

  // Lock the mutex
  mtx.lock();
  int result = rx_resources.create_chunks(d_centre_freq, d_bandwidth, u_id);
  mtx.unlock();

  return result;
}

int
xvl_resource_manager::reserve_tx_resources(unsigned int u_id,
                                           double d_centre_freq,
                                           double d_bandwidth)
{
  // If the TX resources were not defined
  if (not b_transmitter)
  {
    // Cannot reserve TX resources
    return 1;
  }

  // Lock the mutex
  mtx.lock();
  int result = tx_resources.create_chunks(d_centre_freq, d_bandwidth, u_id);
  mtx.unlock();

  return result;
}

boost::property_tree::ptree
xvl_resource_manager::query_resources()
{
  // If the RX/TX resources were no defined
  if ((not b_receiver) and (not b_transmitter))
  {
    std::cerr << "Must define set of resources!" << std::endl;
    exit(30);
  }

  // External an internal trees
  boost::property_tree::ptree tree;

  // Lock the mutex
  mtx.lock();

  // Add the chunks to the tree
  if (b_receiver)
    tree.add_child("receiver", rx_resources.list_chunks());

  if (b_transmitter)
    tree.add_child("transmitter", tx_resources.list_chunks());

  // Unlock the mutex
  mtx.unlock();

  return tree;
}

int
xvl_resource_manager::free_tx_resources(size_t u_id)
{
  // TODO should be able to return an error message
  // If the RX/TX resources were no defined
  if (not b_transmitter)
    {
      std::cerr << "Must define set of resources!" << std::endl;
      exit(30);
  }

  // Lock the mutex
  mtx.lock();
  tx_resources.delete_chunks(u_id);
  mtx.unlock();

  // Return false if succeeded
  return 0;
}

int
xvl_resource_manager::free_rx_resources(size_t u_id)
{
  if (not b_receiver)
  {
    std::cerr << "Must define set of resources!" << std::endl;
    exit(30);
  }

  mtx.lock();
  rx_resources.delete_chunks(u_id);
  mtx.unlock();

  // Return false if succeeded
  return 0;
}

int
xvl_resource_manager::check_tx_free(double d_centre_freq,
                                    double d_bandwidth,
                                    size_t u_id)
{
  rf_front_end tmp = tx_resources;
  tmp.delete_chunks(u_id);
  return  tmp.check_chunk(d_centre_freq, d_bandwidth);
}


int
xvl_resource_manager::check_rx_free(double d_centre_freq,
                                    double d_bandwidth,
                                    size_t u_id)
{
  rf_front_end tmp = rx_resources;
  tmp.delete_chunks(u_id);
  return  tmp.check_chunk(d_centre_freq, d_bandwidth);
}


// Methods of the RF front-end class
rf_front_end::rf_front_end(double d_cf,
                           double d_bw,
                           unsigned int u_id)
{
  // Create empty chunk with all the available resources
  resources.emplace_back(d_cf, d_bw, u_id);
}

int
rf_front_end::create_chunks(double d_centre_freq,
                            double d_bandwidth,
                            unsigned int u_id)
{
  // Hold the result - One if bad
  int result = 1;

  // Iterate over the list of chunks
  for (auto it = resources.begin(); it != resources.end(); it++)
  {
    // Jumps to next chunk if the current chunk isn't free
    if (it->u_id != 0) {continue;}

    // Jump to next chunk if this doesn't fit
    if ((2 * it->d_centre_freq - it->d_bandwidth > 2 * d_centre_freq - d_bandwidth) or
        (2 * it->d_centre_freq + it->d_bandwidth < 2 * d_centre_freq + d_bandwidth)) {continue;}

    /* If there's free space in the lower band */
    if (2 * it->d_centre_freq - it->d_bandwidth < 2 * d_centre_freq - d_bandwidth)
    {
      double l_bandwidth = (d_centre_freq - it->d_centre_freq) - 0.5 * (d_bandwidth - it->d_bandwidth);
      double l_centre_freq = d_centre_freq - 0.5 * (d_bandwidth + l_bandwidth);
      // If this is the first chunk
      if (it == resources.begin())
      {
        //Insert a new free chunk before the current one
        resources.emplace_front(l_centre_freq, l_bandwidth);
      }
      // Otherwise, there are other chunks
      else
      {
        // Check if the lower chunk is free
        auto pv = std::prev(it, 1);
        if (pv->u_id == 0)
        {
          // Update the centre frequency as the geometric mean between the current free chunk and the new free region
          pv->d_centre_freq = sqrt(0.5 * (pv->d_centre_freq * pv->d_bandwidth + l_centre_freq * l_bandwidth));
          pv->d_bandwidth += l_bandwidth;
        }
        else
        {
          resources.emplace(it, l_centre_freq, l_bandwidth);
        }

      }
    }

    // If there's free space in the upper band
    if (2 * it->d_centre_freq + it->d_bandwidth > 2 * d_centre_freq + d_bandwidth)
    {
      double r_bandwidth = (it->d_centre_freq - d_centre_freq) + 0.5 * (it->d_bandwidth - d_bandwidth);
      double r_centre_freq =  d_centre_freq + 0.5 * (d_bandwidth + r_bandwidth);

      // If this is the last chunk
      if (it == --resources.end())
      {
        //Insert a new free chunk after the current one
        resources.emplace_back(r_centre_freq, r_bandwidth);
      }
      // Otherwise, there are other chunks
      else
      {
        // Check if the upper chunk is free
        auto nx = std::next(it, 1);
        if (nx->u_id == 0)
        {
          // Update the centre frequency as the geometric mean between the current free chunk and the new free region
          nx->d_centre_freq = sqrt(0.5 * (nx->d_centre_freq * nx->d_bandwidth + r_centre_freq * r_bandwidth));
          nx->d_bandwidth += r_bandwidth;
        }
        // If not, add a new free chunk after this one
        else
        {
          resources.emplace(nx, r_centre_freq, r_bandwidth);
        }

      }
    } // End upper band

    // Create a chunk for the given service
    (*it) = chunk(d_centre_freq, d_bandwidth, u_id);

    // Change the result flag -- zero is a great signal
    result = 0;
    break;

  } // End for loop

  return result;
}


bool
rf_front_end::check_chunk(double d_centre_freq,
                          double d_bandwidth)
{
  auto lo_freq = [](double cf, double bw) { return cf - bw/2.0;};
  auto hi_freq = [](double cf, double bw) { return cf + bw/2.0;};

  for (auto it = resources.begin(); it != resources.end(); ++it)
  {
    if (lo_freq(d_centre_freq, d_bandwidth) >= lo_freq(it->d_centre_freq, it->d_bandwidth) &&
        hi_freq(d_centre_freq, d_bandwidth) <= hi_freq(it->d_centre_freq, it->d_bandwidth))

      return true;
  }

  return false;
}


boost::property_tree::ptree
rf_front_end::list_chunks()
{
  // Container to hold the chunks
  boost::property_tree::ptree chunks;
  // Iterate over the list of chunks
  for (auto it = resources.begin(); it != resources.end(); ++it)
   {
    // Append the chunk description to the message
    chunks.push_back(std::make_pair("" ,it->output()));
  }
  return chunks;
}

void
rf_front_end::delete_chunks(unsigned int u_id)
{
  // Iterate over the list of chunks
  for (auto it = resources.begin(); it != resources.end(); it++)
  {
    // Jumps to next chunk if the current chunk isn't the right one
    if (it->u_id != u_id) { continue; }

    // Iterators to the previous and next chunks
    auto pv = std::prev(it, 1);
    auto nx = std::next(it, 1);

    // If the neighbouring chunks are not free, or if this is the only chunk
    if ((resources.size() == 1) or
        (pv->u_id != 0 and nx->u_id != 0) or
        (it == resources.begin() and nx->u_id != 0) or
        (it == --resources.end() and pv->u_id != 0))
    {
      // Create a new free the chunk
      (*it) = (chunk) {it->d_centre_freq, it->d_bandwidth, 0};

      // Jump to next iteration
      continue;
    }

    // Temp variables to hold the free chunks' info
    double d_c_centre_freq = it->d_centre_freq;
    double d_c_bandwidth = it->d_bandwidth;

    // If the lower chunk if free
    if (pv->u_id == 0)
    {
      d_c_centre_freq = ((pv->d_centre_freq - pv->d_bandwidth/2.0) + (d_c_centre_freq + d_c_bandwidth/2)) / 2.0 ;
      d_c_bandwidth += pv->d_bandwidth;

      // Erase the neighbour chunk
      resources.erase(pv);
    }

    // If the upper chunk if free
    if (nx->u_id == 0)
    {
      d_c_centre_freq = ((nx->d_centre_freq + nx->d_bandwidth/2) + (d_c_centre_freq - d_c_bandwidth/2))/2.0;
      d_c_bandwidth += nx->d_bandwidth;

      // Erase the neighbour chunk
      resources.erase(nx);
    }

    // Create the new free chunk
    (*it) = chunk(d_c_centre_freq, d_c_bandwidth, 0);

  } // End loop
}

} // namespace hydra

// Test module
#if 1
int main(int argv, char **argc)
{
  using namespace hydra;

  double cf = 1.1e6;
  double bw = 2e6;
  int workd;

  xvl_resource_manager rm;
  rm.set_rx_resources(cf, bw);

  // Tree to get respondes from the methods
  boost::property_tree::ptree output_tree;
  // String stream to pipe the trees
  std::stringstream es;

  std::cout << "\nInitial setup" << std::endl;
  output_tree = rm.query_resources();
  boost::property_tree::json_parser::write_json(es, output_tree);
  std::cout << es.str() << std::endl;

  es.str("");
  std::cout << boost::format("Allocated first chunk @%1%, %2%") % (cf-200e3) % 200e3 << std::endl;
  workd = rm.reserve_rx_resources(90, cf - 200e3, 200e3);
  output_tree = rm.query_resources();
  boost::property_tree::json_parser::write_json(es, output_tree);
  std::cout << es.str() << std::endl;

  es.str("");
  std::cout << boost::format("Allocated second chunk @%1%, %2%") % (cf+200e3) % 100e3 << std::endl;
  workd = rm.reserve_rx_resources(91, cf + 200e3, 100e3);
  output_tree = rm.query_resources();
  boost::property_tree::json_parser::write_json(es, output_tree);
  std::cout << es.str() << std::endl;

  es.str("");
  std::cout << "\nFreed first chunk" << std::endl;
  workd = rm.free_rx_resources(90);
  output_tree = rm.query_resources();
  boost::property_tree::json_parser::write_json(es, output_tree);
  std::cout << es.str() << std::endl;

  es.str("");
  std::cout << "\nFreed first chunk" << std::endl;
  workd = rm.free_rx_resources(91);
  output_tree = rm.query_resources();
  boost::property_tree::json_parser::write_json(es, output_tree);
  std::cout << es.str() << std::endl;


#if 0
  rm.set_rx_resources(2e3, 20);
  int workd;

  // Tree to get respondes from the methods
  boost::property_tree::ptree output_tree;
  // String stream to pipe the trees
  std::stringstream es;

  std::cout << "\nInitial setup" << std::endl;
  output_tree = rm.query_resources();
  boost::property_tree::json_parser::write_json(es, output_tree);
  std::cout << es.str() << std::endl;

  es.str("");
  std::cout << "\nAllocated first chunk" << std::endl;
  workd = rm.reserve_rx_resources(3, 2e3, 10);
  output_tree = rm.query_resources();
  boost::property_tree::json_parser::write_json(es, output_tree);
  std::cout << es.str() << std::endl;

  es.str("");
  std::cout << "\nFreed first chunk" << std::endl;
  workd = rm.free_resources(3);
  output_tree = rm.query_resources();
  boost::property_tree::json_parser::write_json(es, output_tree);
  std::cout << es.str() << std::endl;


  es.str("");
  std::cout << "\nAllocated lower chunk" << std::endl;
  workd = rm.reserve_rx_resources(2, 1992.5, 5);
  output_tree = rm.query_resources();
  boost::property_tree::json_parser::write_json(es, output_tree);
  std::cout << es.str() << std::endl;

  es.str("");
  std::cout << "\nAllocated upper chunk" << std::endl;
  workd = rm.reserve_rx_resources(1 , 2007.5, 5);
  output_tree = rm.query_resources();
  boost::property_tree::json_parser::write_json(es, output_tree);
  std::cout << es.str() << std::endl;

  es.str("");
  std::cout << "\nTrying to free lower chunks" << std::endl;
  workd = rm.free_resources(2);
  output_tree = rm.query_resources();
  boost::property_tree::json_parser::write_json(es, output_tree);
  std::cout << es.str() << std::endl;


  es.str("");
  std::cout << "\nTrying to free upper chunks" << std::endl;
  workd = rm.free_resources(1);
  output_tree = rm.query_resources();
  boost::property_tree::json_parser::write_json(es, output_tree);
  std::cout << es.str() << std::endl;
#endif

  return 0;
}
#endif
