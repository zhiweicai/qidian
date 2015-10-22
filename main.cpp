#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/utility/string_ref.hpp"

#include <chrono>
#include <thread>
#include <network/uri.hpp>
#include <algorithm>


using namespace boost;
using boost::asio::ip::tcp;

class client
{
public:
  client(boost::asio::io_service& io_service,
      const std::string& server, const std::string& path)
    : resolver_(io_service),
      socket_(io_service)
  {
    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    
    m_bConnected = false;  
    m_server = server;

      
    std::ostringstream request_stream;
    request_stream << "GET " << path << " HTTP/1.0\r\n";
    request_stream << "Host: " << server << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";    
    m_request = request_stream.str();
  }
  
 
void connect ()
{
  // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.
    tcp::resolver::query query(m_server, "http");
    auto endpoint = resolver_.resolve (query);    
    
    boost::system::error_code ec;    
    boost::asio::connect (socket_,endpoint,ec);    
    if (ec)
    {
      std::cout << "failed to connect"<< std::endl;
      return;
    }    
    else
    {  
      std::cout << "connected" << std::endl;
      m_bConnected = true;
    }
}   


void slowsend ()
{
     
   boost::system::error_code error;  
   
  if (m_bConnected)
  {
    
      boost::asio::ip::tcp::no_delay option(true);
      socket_.set_option(option);
      
   //   boost::asio::socket_base::keep_alive option2(true);
   //   socket_.set_option(option2);

      const char* pMsg = m_request.c_str();
      
      int nBlockSize = 20;
      
      int nSegment = (m_request.length () - 1)/nBlockSize;      
      
      int nNeedToSend = std::min (nBlockSize,(int)m_request.length () - 1);
      
      for (int i=0; i<= nSegment;++i)
      {
        boost::asio::write(socket_, boost::asio::buffer(pMsg+i*nBlockSize,nNeedToSend),error);  
        if(error) 
        {
           std::cout << "send failed: " << error.message() << std::endl;
           return;
        }
   
        
            
        int nRemaining = (m_request.length () - 1) - (i+1) * nBlockSize;
        
        if (nRemaining == 0)
           break;        
        else if (nRemaining >=nBlockSize) 
            nNeedToSend = nBlockSize;
        else
            nNeedToSend = nRemaining;      
            
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));        
                
      }
      
      // now let us send the last character 
      std::this_thread::sleep_for(std::chrono::milliseconds(5000));      
      boost::asio::write(socket_, boost::asio::buffer(pMsg+m_request.length () - 1,1),error); 
      
      
      boost::asio::read(socket_, response_, boost::asio::transfer_all(), error );

      if( error && error != asio::error::eof )  
      {
        std::cout << "receive failed: " << error.message() << std::endl;
        return;
      }
      else
      {
        const char* data = boost::asio::buffer_cast<const char*>(response_.data());
        std::cout << data << std::endl;
      }     
   }       
      
}


private:
  tcp::resolver resolver_;
  tcp::socket socket_;
  boost::asio::streambuf response_;
  bool m_bConnected;
  std::string m_server;
  std::string m_request;
};


void StartConnecting (const std::string& sStartTime, const std::string& sHost, const std::string& sPath);
std::string GetHostIPAddress (const std::string& sHost);
void StartMonitoring (const std::string& sHost);

int main (int argc, char* argv[])
{	
	if (argc <3)
	{	
	   std::cout << "Please specify URL and starttime" << std::endl;
	   return -1;
	}
	
	// get the url from the command line	
	std::string sURL = argv[1];
  
  network::uri  baseURI (sURL);
  auto sHost_ = baseURI.host ().get();
  auto sPath_ = baseURI.path ().get();
  
  std::string sHost (sHost_.begin(),sHost_.size ());
  std::string sPath (sPath_.begin(),sPath_.size ());
  
  std::cout << sHost << std::endl;
  std::cout << sPath << std::endl;
	
	// get the hostname from the url
	
	std::string sIP = GetHostIPAddress (sHost);
	
	// get the start time 	
  std::string sStartTime = argv[2];
	   
	// Establish connection to the url
	StartConnecting (sStartTime,sHost, sPath);
	
	// Start monitoring
	StartMonitoring (sIP);  

}



std::string GetHostIPAddress (const std::string& sURL)
{
	std::cout << "IP address" << std::endl;
	
	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(sURL,"http");
	
	tcp::endpoint remote_endpoint = *resolver.resolve(query);
	
	std::cout << remote_endpoint.address().to_string() << std::endl;	
	
	return remote_endpoint.address().to_string();
}



void StartConnecting (const std::string& sStartTime, const std::string& sHost, const std::string& sPath)
{
	
	// sleep until the start time
	
	namespace pt = boost::posix_time;
	
	using namespace pt;
		
	ptime now = microsec_clock::local_time();
			
	ptime startTime (now.date(),duration_from_string (sStartTime));
		
	time_duration diff = startTime - now;
	long x = diff.total_milliseconds();
	
	std::cout << x << std::endl;
	
	std::this_thread::sleep_for(std::chrono::milliseconds(x));
	
	
	
	// establish multiple connections to the website
	
   try
   {
       boost::asio::io_service io_service;
       client c(io_service,sHost, sPath);
       c.connect ();
       c.slowsend ();
     
   }
   catch (std::exception& e)
   {
       std::cout << "Exception: " << e.what() << "\n";
   }
	
	
	// slow sending the request to the target url
	

	
	// adjust the time according to the time server
	
	// send the last chunk of data simulatious from different task sequentially
	
	
	// parse the returned result
}


void StartMonitoring (const std::string& sHost)
{
   // monitor the ack from the server to know the response time and save it to the database
		
	return;
}