#ifndef PSALG_SEGGEOMETRYMATRIXV1_H
#define PSALG_SEGGEOMETRYMATRIXV1_H

//-------------------

#include "psalg/geometry/SegGeometry.hh"
#include <string>
#include <map>

namespace geometry {

/// @addtogroup geometry

/**
 *  @ingroup geometry
 *
 *  @brief Class SegGeometryMatrixV1 defines the matrix V1 (pnCCD, 512x512) sensor pixel coordinates in its local frame.
 *
 *  Default constructor parameters are set for pnCCD; 512x512 pixels with 75x75um pixel size.
 *
 *
 *  MatrixV1 sensor coordinate frame has a matrix-style coordinate system:
 * 
 *  @code
 *    (Xmin,Ymin)        (Xmin,Ymax)
 *    (0,0)              (0,511)
 *       +-----------------+----> Y
 *       |                 |
 *       |                 |
 *       |                 |
 *       |                 |
 *       |                 |
 *       |                 |
 *       |                 |
 *       |                 |
 *       +-----------------+
 *       |
 *     X V
 *    (511,0)           (511,511)
 *    (Xmax,Ymin)       (Xmax,Ymax)
 *  @endcode
 *
 *  Pixel (r,c)=(0,0) is in the top left corner of the matrix which has coordinates (Xmin,Ymin) - is in origin.
 *  Here we assume that sensor has 512 rows and 512 columns.
 *   
 *  @anchor interface
 *  @par<interface> Interface Description
 * 
 *  @li  Include and typedef
 *  @code
 *  #include "psalg/geometry/SegGeometryMatrixV1.hh"
 *  typedef geometry::SegGeometryMatrixV1 SG;
 *  @endcode
 *
 *  @li  Instatiation
 *  @code
 *       SG *seg_geom = new SG();  
 *  or
 *       bool use_wide_pix_center = true;
 *       SG *seg_geom = new SG(use_wide_pix_center);  
 *  @endcode
 *
 *  @li  Print info
 *  @code
 *       bitword_t pbits=0377; // 1-member data; 2-coordinate arrays; 4-min/max coordinate values
 *       seg_geom -> print_seg_info(pbits);
 *  @endcode
 *
 *  @li  Access methods
 *  @code
 *        // scalar values
 *        const gsize_t         array_size        = seg_geom -> size(); // 512*512
 *        const gsize_t         number_of_rows    = seg_geom -> rows(); // 512
 *        const gsize_t         number_of_cols    = seg_geom -> cols(); // 512
 *        const pixel_coord_t  pixel_scale_size  = seg_geom -> pixel_scale_size();             // 75.00 
 *        const pixel_coord_t  pixel_coord_min   = seg_geom -> pixel_coord_min(SG::AXIS_Z);
 *        const pixel_coord_t  pixel_coord_max   = seg_geom -> pixel_coord_max(SG::AXIS_X);
 * 
 *        // pointer to arrays with size equal to array_size
 *        const gsize_t*        p_array_shape     = seg_geom -> shape();                        // {512, 512}
 *        const pixel_area_t*  p_pixel_area      = seg_geom -> pixel_area_array(); // array of 1-for regular or 2.5-for long pixels
 *        const pixel_coord_t* p_pixel_size_arr  = seg_geom -> pixel_size_array(SG::AXIS_X);
 *        const pixel_coord_t* p_pixel_coord_arr = seg_geom -> pixel_coord_array(SG::AXIS_Y);
 *
 *        bitword_t mbits=0377; // 1-edges; 2-wide central cols; 4-non-bound; 8-non-bound neighbours
 *        const pixel_mask_t*  p_mask_arr = seg_geom -> pixel_mask_array(mbits);
 *  @endcode
 *  
 *  This software was developed for the LCLS project.
 *  If you use all or part of it, please give an appropriate acknowledgment.
 *
 *  @author Mikhail S. Dubrovin
 */ 

//-------------------
  /**  
   *  @brief Splits the string segname like MTRX:384:384:100:100 and returns values
   *  @param[in] segname - string like MTRX:384:384:100:100;
   *  @param[out] rows - number of rows
   *  @param[out] cols - number of columnss
   *  @param[out] pix_size_rows - pixel size along axis counting rows
   *  @param[out] pix_size_cols - pixel size along axis counting cols
   */  
bool matrix_pars(const std::string& segname
		,gsize_t& rows
		,gsize_t& cols
		,pixel_coord_t& pix_size_rows
		,pixel_coord_t& pix_size_cols);

//-------------------

class SegGeometryMatrixV1 : public geometry::SegGeometry {

public:

  typedef std::map<const std::string, geometry::SegGeometry*> MapInstance;

  /// Number of corners
  static const gsize_t  NCORNERS = 4;

  /// Pixel scale size [um] for indexing  
  //static constexpr pixel_coord_t PIX_SCALE_SIZE_DEF = 12345;
  static const pixel_coord_t PIX_SCALE_SIZE_DEF;// = 12345;

  /// Pixel size [um] in column direction
  static const pixel_coord_t PIX_SIZE_COLS_DEF;// = 75;

  /// Pixel size [um] in row direction
  static const pixel_coord_t PIX_SIZE_ROWS_DEF;// = 75;

  /// Pixel size [um] in depth
  static const pixel_coord_t PIX_SIZE_DEPTH_DEF;// = 400.;

  /**
   *  @brief Fills-in the map of perfect segment coordinates, defined through the chip geometry.
   *  @param[in] rows - number of rows
   *  @param[in] cols - number of columns
   *  @param[in] pix_size_rows - pixel size in rows in micrometers [um] 
   *  @param[in] pix_size_cols - pixel size in columns in micrometers [um] 
   *  @param[in] pix_size_depth - pixel size in depth in micrometers [um] 
   *  @param[in] pix_scale_size - pixel size parameter taken as a 2-d image grid parameter
   */
  SegGeometryMatrixV1(const gsize_t& rows = 512
		     ,const gsize_t& cols = 512
		     ,const pixel_coord_t& pix_size_rows  = PIX_SIZE_ROWS_DEF
		     ,const pixel_coord_t& pix_size_cols  = PIX_SIZE_COLS_DEF
		     ,const pixel_coord_t& pix_size_depth = PIX_SIZE_DEPTH_DEF
		     ,const pixel_coord_t& pix_scale_size = PIX_SCALE_SIZE_DEF
                     );

  virtual ~SegGeometryMatrixV1();

  //-----------------
  /// Implementation of interface methods

  /// Prints segment info for selected bits
  virtual void print_seg_info(const bitword_t& pbits=0);

  /// Returns size of the coordinate arrays
  virtual const gsize_t size() {return SIZE;}

  /// Returns number of rows in segment
  virtual const gsize_t rows() {return ROWS;}

  /// Returns number of cols in segment
  virtual const gsize_t cols() {return COLS;}

  /// Returns shape of the segment {rows, cols}
  virtual const gsize_t* shape() {return &ARR_SHAPE[0];}

  /// Returns pixel size in um for indexing
  virtual const pixel_coord_t pixel_scale_size() {return PIX_SCALE_SIZE;}

  /// Returns pointer to the array of pixel areas
  virtual const pixel_area_t* pixel_area_array();

  /**  
   *  @brief Returns pointer to the array of pixel size in um for AXIS
   *  @param[in] axis       Axis from the enumerated list for X, Y, and Z axes
   */
  virtual const pixel_coord_t* pixel_size_array(AXIS axis);

  /// Returns pointer to the array of segment pixel coordinates in um for AXIS
  virtual const pixel_coord_t* pixel_coord_array(AXIS axis);

  /// Returns minimal value in the array of segment pixel coordinates in um for AXIS
  virtual const pixel_coord_t pixel_coord_min(AXIS axis);

  /// Returns maximal value in the array of segment pixel coordinates in um for AXIS
  virtual const pixel_coord_t pixel_coord_max(AXIS axis);

  /**  
   *  @brief Returns pointer to the array of pixel mask: 1/0 = ok/masked
   *  @param[in] mbits - mask control bits;
   *             + 1 - mask edges,
   */  
  virtual const pixel_mask_t* pixel_mask_array(const bitword_t& mbits = 0377);

  //-----------------
  // Singleton stuff:

  static geometry::SegGeometry* instance(const std::string& segname="MTRX:512:512:75:75");

private:

  SegGeometryMatrixV1(const std::string& segname="MTRX:512:512:75:75"); // def - pnccd segment

  //static geometry::SegGeometry* m_pInstance;

  static MapInstance _map_segname_instance;

  //-----------------

  /// Generator of the pixel coordinate arrays.
  void make_pixel_coord_arrs();

  /// Generator of the pixel size and area arrays.
  void make_pixel_size_arrs();

  /// Prints class member data
  void print_member_data();

  /// Prints segment pixel coordinates
  void print_coord_arrs();

  /// Prints minimal and maximal values of the segment coordinates for X, Y, and Z axes
  void print_min_max_coords();

  /// Number of pixel rows in segment 
  gsize_t ROWS;
  /// Number of pixel columnss in segment
  gsize_t COLS;
  /// Number of pixels in segment SIZE = COLS*ROWS
  gsize_t SIZE;

  gsize_t IND_CORNER[NCORNERS];
  gsize_t ARR_SHAPE[2];

  /// Pixel scale size [um] for indexing  
  pixel_coord_t PIX_SCALE_SIZE; // = 75;

  /// Pixel size [um] in column direction
  pixel_coord_t PIX_SIZE_COLS; //  = 75;

  /// Pixel size [um] in row direction
  pixel_coord_t PIX_SIZE_ROWS; //  = 75;

  /// Pixel size [um] in depth
  pixel_coord_t PIX_SIZE_DEPTH; // = 400.;

  /// Conversion factor between um and pix 
  double UM_TO_PIX; //             = 1./75;

  /// switch between two options of the wide pixel row center
  bool m_use_wide_pix_center;

  /// done bits
  bitword_t m_done_bits;

  /// 1-d pixel coordinates of rows and cols
  pixel_coord_t*  m_x_arr_um;  
  pixel_coord_t*  m_y_arr_um;  

  /// 2-d pixel coordinate arrays
  pixel_coord_t*  m_x_pix_coord_um;  
  pixel_coord_t*  m_y_pix_coord_um;  
  pixel_coord_t*  m_z_pix_coord_um;

  /// 2-d pixel size arrays
  pixel_coord_t*  m_x_pix_size_um;  
  pixel_coord_t*  m_y_pix_size_um;  
  pixel_coord_t*  m_z_pix_size_um;

  /// 2-d pixel area arrays
  pixel_area_t*  m_pix_area_arr;  

  /// 2-d pixel mask arrays
  pixel_mask_t*  m_pix_mask_arr;  

  // Copy constructor and assignment are disabled by default
  SegGeometryMatrixV1(const SegGeometryMatrixV1&) = delete;
  SegGeometryMatrixV1& operator = (const SegGeometryMatrixV1&) = delete;
};

} // namespace geometry

#endif // PSALG_SEGGEOMETRYMATRIXV1_H
