#include<iostream>
#include<fstream>
#include"http/server.hpp"
using namespace fetch::http;

int main() 
{
  fetch::network::ThreadManager tm(1);  
  HTTPServer server(8080, &tm);

  server.AddMiddleware( [](HTTPRequest &req) {
      std::cout << "Middleware 1" << std::endl;      
    });


  server.AddMiddleware( [](HTTPResponse &res, HTTPRequest const &req) {
      std::cout << res.status().code << " " << req.uri() << std::endl;      
    });

  server.AddView( "/", [](ViewParameters const &params, HTTPRequest const &req) {
      HTTPResponse res("Hello world -- this is a render of the view");

      return res;      
    });

  server.AddView( "/pages/(id=\\d+)/", [](ViewParameters const &params, HTTPRequest const &req) {
      HTTPResponse res("Secret page 1");

      return res;      
    });

  server.AddView( "/other/(name=\\w+)", [](ViewParameters const &params, HTTPRequest const &req) {
      HTTPResponse res("Secret page with name");

      return res;      
    });

  server.AddView( "/other/(name=\\w+)/(number=\\d+)", [](ViewParameters const &params, HTTPRequest const &req) {
      HTTPResponse res("Secret page with name and number: " + params["name"] + " and "+ params["number"]);

      return res;      
    });

  server.AddView( "/static/(filename=.+)", [](ViewParameters const &params, HTTPRequest const &req) {

      std::string filename = params["filename"];      
      std::size_t pos = filename.find_last_of('.');      
      std::string ext = filename.substr(pos, filename.size() - pos);
      auto mtype = fetch::http::mime_types::GetMimeTypeFromExtension(ext);
      
      std::cout << mtype.type << std::endl;
      std::fstream fin(filename, std::ios::in | std::ios::binary);
      fin.seekg (0, fin.end);
      int length = fin.tellg();
      fin.seekg (0, fin.beg);

      fetch::byte_array::ByteArray data;
      data.Resize( length );
      fin.read (reinterpret_cast< char*>( data.pointer() ), length);
      
      fin.close();      

      HTTPResponse res(data, mtype );        
      return res;      
    });
  
  
  tm.Start();
  
  std::cout << "Ctrl-C to stop" << std::endl;
  while(true) 
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
  }

  tm.Stop();
  
  return 0;
}
