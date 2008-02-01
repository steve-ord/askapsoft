/// @file
/// 
/// @brief A structural type joining together GenericEquation and MultiChunkEquation
/// @details Because we deal here with double inheritance, we need to overload
/// explicitly predict and calcGenericEquation methods, otherwise the methods
/// in the scimath::Equation tree are left pure abstract. Theoretically, we can
/// get rid of the double inheritance here and the need of this class would 
/// disappear. However, MultiChunkEquation is envisaged to be a temporary
/// class before we start to work with accessors only. Therefore, the inheritance
/// is left double to highlight the future interface 
/// (derived from IMeasurementEquation).
///
/// @copyright (c) 2007 CONRAD, All Rights Reserved.
/// @author Max Voronkov <maxim.voronkov@csiro.au>

#ifndef GENERIC_MULTI_CHUNK_EQUATION_H
#define GENERIC_MULTI_CHUNK_EQUATION_H

// own include
#include <measurementequation/MultiChunkEquation.h>
#include <fitting/GenericEquation.h>
#include <fitting/GenericNormalEquations.h>


namespace conrad {

namespace synthesis {


/// @brief A structural type joining together GenericEquation and MultiChunkEquation
/// @details Because we deal here with double inheritance, we need to overload
/// explicitly predict and calcGenericEquation methods, otherwise the methods
/// in the scimath::Equation tree are left pure abstract. Theoretically, we can
/// get rid of the double inheritance here and the need of this class would 
/// disappear. However, MultiChunkEquation is envisaged to be a temporary
/// class before we start to work with accessors only. Therefore, the inheritance
/// is left double to highlight the future interface 
/// (derived from IMeasurementEquation).
/// @ingroup measurementequation
struct GenericMultiChunkEquation : virtual public MultiChunkEquation,
                              virtual public conrad::scimath::GenericEquation    
{  
  /// @brief Standard constructor, which remembers data iterator.
  /// @param[in] idi data iterator
  GenericMultiChunkEquation(const IDataSharedIter& idi);

  /// @brief Calculate the normal equations for the iterator
  /// @details This version iterates through all chunks of data and
  /// calls an abstract method declared in IMeasurementEquation for each 
  /// individual accessor (each iteration of the iterator)
  /// @param[in] ne Normal equations
  virtual void calcGenericEquations(conrad::scimath::GenericNormalEquations& ne) const;

  /// @brief Predict model visibility for the iterator.
  /// @details This version of the predict method iterates
  /// over all chunks of data and calls an abstract method declared
  /// in IMeasurementEquation for each accessor. 
  virtual void predict() const;      
};


} // namespace synthesis

} // namespace conrad

#endif // #ifndef GENERIC_MULTI_CHUNK_EQUATION_H
