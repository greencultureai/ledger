#pragma once

#include"math/linalg/prototype.hpp"
#include"math/linalg/matrix.hpp"
#include"math/linalg/blas/base.hpp"

namespace fetch
{
namespace math
{
namespace linalg 
{


template<typename S>
class Blas< S, 
            Signature( _C <= _alpha, _A, _B, _beta, _C ),
            Computes( _C = _alpha * _A * _B + _beta * _C ), 
            platform::Parallelisation::THREADING>
{
public:
  using vector_register_type = typename Matrix< S >::vector_register_type;
  using type = S;
  
  void operator()(type const &alpha, Matrix< type > const &a, Matrix< type > const &b, type const &beta, Matrix< type > &c ) 
  {
    std::size_t j;
    std::size_t i;
    if( (c.height() == 0) || ((c.width() == 0) || (((alpha == 0.0) || (a.width() == 0)) && (beta == 1.0))) ) 
    {
      return;
    } 
    
    if( alpha == 0.0 ) 
    {
      if( beta == 0.0 ) 
      {
        for(j = 0 ; j <  c.width(); ++j )
        {
          for(i = 0 ; i <  c.height(); ++i )
          {
            c(i, j) = 0.0;
          }
        }
      }
      else 
      {
        for(j = 0 ; j <  c.width(); ++j )
        {
          for(i = 0 ; i <  c.height(); ++i )
          {
            c(i, j) = beta * c(i, j);
          }
        }
      } 
      
      return;
    } 
    
    
    for(j = 0 ; j < c.width(); ++j )
    {
      pool_.Dispatch([j, alpha, a, b, beta, &c]() {
        std::size_t i; 
            std::size_t l;
        if( beta == 0.0 ) 
        {
          for(i = 0 ; i <  c.height(); ++i )
          {
            c(i, j) = 0.0;
          }
        }
        else if ( beta != 1.0 )
        {
          for(i = 0 ; i <  c.height(); ++i )
          {
            c(i, j) = beta * c(i, j);
          }
        } 
        
        for(l = 0 ; l <  a.width(); ++l )
        {
          double temp;
          temp = alpha * b(l, j);
          for(i = 0 ; i <  c.height(); ++i )
          {
            c(i, j) = c(i, j) + temp * a(i, l);
          }
        }
      });
    
      pool_.Wait();}
    return;
    
  }
private:
  threading::Pool pool_;
};


} // namespace linalg
} // namespace math
} // namepsace fetch