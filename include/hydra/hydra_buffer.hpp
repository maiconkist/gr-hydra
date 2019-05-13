#ifndef INCLUDED_HYDRA_BUFFER_H
#define INCLUDED_HYDRA_BUFFER_H

#include <queue>
#include <vector>
#include <thread>
#include <mutex>

#include <boost/circular_buffer.hpp>
#include "hydra/types.h"


namespace hydra {

// Template to allow different data types and container types
template <typename data_type, template<typename, typename> class container_type = boost::circular_buffer>
class hydra_buffer
{
  private:
    // The actual buffer, a container of the given type of data
    container_type<data_type, std::allocator<data_type>> buffer;
    // Output mutex
    std::mutex buffer_mutex;
    // Temporary element container
    data_type temp_element;
    // Temporary element vector container
    std::vector<data_type> temp_elements;

  public:
    // Default constructor
    hydra_buffer();

    // Class Constructor
    hydra_buffer(unsigned int size);

    // Return the current size of the buffer
    const unsigned int size()
    {
      // Lock access to the inner buffer structure
      std::lock_guard<std::mutex> lock(buffer_mutex);

      return buffer.size();
    };

    // Return the capacity of the buffer
    const unsigned int capacity(){return buffer.capacity();};

    // Read a number of elements
    std::vector<data_type> read(const unsigned int &num_elements = 1);

    // Read a single elements
    data_type read_one();

    // Write a number of the same element in the buffer
    void write(const data_type &element, const unsigned int &num_elements = 1);

    // Write a number of elements to the buffer
    template <typename iterator>
    void write(iterator begin_it, const unsigned int &num_elements = 1);

    // Write a range of elements to the buffer
    template <typename iterator>
    void write(iterator begin_it, iterator end_it);

    // Access operator
    data_type operator[](const unsigned int &position);
};


template <typename data_type, template<typename, typename> class container_type>
hydra_buffer<data_type, container_type>::hydra_buffer(){};

template <typename data_type, template<typename, typename> class container_type>
hydra_buffer<data_type, container_type>::hydra_buffer(unsigned int size)
{
  // Allocate a given size for the buffer
  buffer = container_type<data_type, std::allocator<data_type>> (size);
}

// Read a number of elements
template <typename data_type, template<typename, typename> class container_type>
data_type
hydra_buffer<data_type, container_type>::read_one()
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // If we have at least one elemen to be consumed
  if (buffer.size())
  {
    // Copy the first element
    temp_element = buffer.front();
    // Remove the first element from the buffer
    buffer.pop_front();

    // Return the element
    return temp_element;
  }
  // Ops, not there yet
  else
  {
    // Return a default constructed instance, i.e., empty vector
    return {};
  }

}


// Read a number of elements
template <typename data_type, template<typename, typename> class container_type>
std::vector<data_type>
hydra_buffer<data_type, container_type>::read(const unsigned int &num_elements)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // If we have enough elemens to be consumed
  if (buffer.size() >= num_elements)
  {
    // Create an array to hold the elements
    temp_elements.resize(num_elements);

    // Copy the given number of elements to the temp array
    std::copy(buffer.begin(), buffer.begin()+num_elements, temp_elements.begin());
    // Remove the given numbert of elements from the buffer
    buffer.erase_begin(num_elements);

    // Return the array of elements
    return temp_elements;
  }
  // Ops, not there yet
  else
  {
    // Return a default constructed instance, i.e., empty vector
    return {};
  }

}

// Write a number of the same element in the buffe:
template <typename data_type, template<typename, typename> class container_type>
void
hydra_buffer<data_type, container_type>::write(const data_type &element, const unsigned int &num_elements)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // If the writting process will not overflow/overwrite the buffer
  if (buffer.size() + num_elements <= buffer.capacity())
  {
    // Insert N elements at the end
    buffer.insert(buffer.end(), num_elements, element);
  }
}

// Write a number of elements to the buffer
template <typename data_type, template<typename, typename> class container_type>
template <typename iterator>
void
hydra_buffer<data_type, container_type>::write(iterator begin_it, const unsigned int &num_elements)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // If the writting process will not overflow/overwrite the buffer
  if (buffer.size() + num_elements <= buffer.capacity())
  {
    // Assign N elements at the end, starting from the begin iterator
    buffer.insert(buffer.end(), begin_it, begin_it+num_elements);
  }
}

// Write a range of elements to the buffer
template <typename data_type, template<typename, typename> class container_type>
template <typename iterator>
void
hydra_buffer<data_type, container_type>::write(iterator begin_it, iterator end_it)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // If the writting process will not overflow/overwrite the buffer
  if (buffer.size() + std::distance(begin_it, end_it) <= buffer.capacity())
  {
    // Assign a range of elements at the end, between the begin and end iterators
    buffer.insert(buffer.end(), begin_it, end_it);
  }
}

// Access operator
template <typename data_type, template<typename, typename> class container_type>
data_type
hydra_buffer<data_type, container_type>::operator[](const unsigned int &position)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // Return element at a given position
  return buffer[position];
}


} // Namespace
#endif
