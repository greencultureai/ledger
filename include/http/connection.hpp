#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP
#include"http/http_connection_manager.hpp"
#include "http/abstract_connection.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "mutex.hpp"
#include "assert.hpp"
#include "logger.hpp"

#include <asio.hpp>
#include <memory>
#include <deque>
namespace fetch {
namespace http {

class HTTPConnection : public AbstractHTTPConnection,
                       public std::enable_shared_from_this<HTTPConnection> 
{
public:
  typedef std::deque< HTTPResponse > response_queue_type;
  typedef typename AbstractHTTPConnection::shared_type connection_type;  
  typedef HTTPConnectionManager::handle_type handle_type;
  typedef std::shared_ptr< HTTPRequest > shared_request_type;
  typedef std::shared_ptr< asio::streambuf > buffer_ptr_type;
  
  
  HTTPConnection(asio::ip::tcp::tcp::socket socket, HTTPConnectionManager& manager)
    : socket_(std::move(socket)),
      manager_(manager),
      write_mutex_(__LINE__, __FILE__)
  {
    fetch::logger.Debug("HTTP connection from ", socket_.remote_endpoint().address().to_string() );
  }

  ~HTTPConnection() 
  {
    manager_.Leave(handle_);
  }
  
  void Start() 
  {
    is_open_ = true;    
    handle_ = manager_.Join(shared_from_this());
    if(is_open_) ReadHeader();
  }

  void Send(HTTPResponse const&response) override 
  {
    write_mutex_.lock();
    bool write_in_progress = !write_queue_.empty();  
    write_queue_.push_back(response);
    write_mutex_.unlock();
    
    if (!write_in_progress) 
    {
      Write();
    }    
  }
  
  
  std::string Address() override
  {
    return socket_.remote_endpoint().address().to_string();    
  }

  asio::ip::tcp::tcp::socket& socket() { return socket_; }
  
public:
  void ReadHeader(buffer_ptr_type buffer_ptr = nullptr) 
  {
    fetch::logger.Debug("Ready to ready HTTP header");
    
    shared_request_type request = std::make_shared< HTTPRequest >();
    if(!buffer_ptr ) buffer_ptr = std::make_shared< asio::streambuf  >(  std::numeric_limits<std::size_t>::max() );

    auto self = shared_from_this();
    
    auto cb = [this, buffer_ptr, request, self](std::error_code const &ec, std::size_t const &len) {
      fetch::logger.Debug("Read HTTP header");
      
      if(ec) {        
        this->HandleError(ec, request);
        return;          
      }
      else
      {
        
        HTTPRequest::SetHeader( *request, *buffer_ptr,  len );        
        if(is_open_) ReadBody( buffer_ptr, request );          
      }
    };

    asio::async_read_until(socket_, *buffer_ptr, "\r\n\r\n", cb);        
  }

  void ReadBody(buffer_ptr_type buffer_ptr, shared_request_type request) 
  {
    
    if( request->content_length() <= buffer_ptr->size() ) {
      HTTPRequest::SetBody( *request, *buffer_ptr);

      manager_.PushRequest(handle_,  *request ); 
      
      if(is_open_) ReadHeader(buffer_ptr); 
      return;      
    }

    auto self = shared_from_this();    
    auto cb = [this, buffer_ptr, request, self](std::error_code const &ec, std::size_t const &len) {
      if(ec) {
        this->HandleError(ec, request);
        return;
      }
      
    };
    
    std::cout << "reading remaining" << std::endl;
    std::cout << "TODO: check the internals of async read" << std::endl;    
    asio::async_read(socket_, *buffer_ptr, asio::transfer_exactly( request->content_length() - buffer_ptr->size()), cb);    
  }
  

  void HandleError(std::error_code const &ec, shared_request_type req) 
  {
    std::stringstream ss;
    ss << ec;    
    fetch::logger.Debug("HTTP error: ", ss.str());

    Close();    
  }
  
  
  void Write() 
  {
    buffer_ptr_type buffer_ptr = std::make_shared< asio::streambuf  >(  std::numeric_limits<std::size_t>::max() );
    write_mutex_.lock();
    HTTPResponse res = write_queue_.front();
    write_queue_.pop_front();
    write_mutex_.unlock();
    
    HTTPResponse::WriteToBuffer(res, *buffer_ptr);    
    auto self = shared_from_this();        
    auto cb = [this, self, buffer_ptr](std::error_code ec, std::size_t) 
      {
        if (!ec) 
        {
          write_mutex_.lock();
          bool write_more = !write_queue_.empty();
          write_mutex_.unlock();  
          if (is_open_ && write_more) 
          {
            Write();
          }
        }
        else 
        {
          manager_.Leave(handle_);
        }
      };


    asio::async_write(socket_, *buffer_ptr, cb);    
  }

  void Close() {
    is_open_ = false;    
    manager_.Leave( handle_ );
  }
  
  asio::ip::tcp::tcp::socket socket_;
  HTTPConnectionManager &manager_;
  response_queue_type write_queue_;  
  fetch::mutex::Mutex write_mutex_;

  handle_type handle_;
  bool is_open_ = false;
  
};

  
};
};


#endif
