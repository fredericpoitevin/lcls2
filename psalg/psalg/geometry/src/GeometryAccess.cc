//-------------------

#include "psalg/geometry/GeometryAccess.hh"

#include <iostream> // cout
#include <fstream>  // ifstream 
#include <iomanip>  // setw, setfill

//-------------------

namespace geometry {

typedef GeometryAccess::image_t image_t;

//-------------------

GeometryAccess::GeometryAccess(const std::string& path)
  : m_path(path)
  , m_pbits(0)
  , p_iX(0)
  , p_iY(0)
  , p_image(0)
  , p_XatZ(0)
  , p_YatZ(0)
{
  MSG(DEBUG, "In c-tor GeometryAccess(const std::string& path) fname=" << path); 
  load_pars_from_file();
}

//--------------

GeometryAccess::GeometryAccess(std::stringstream& ss)
  : m_path("")
  , m_pbits(0)
  , p_iX(0)
  , p_iY(0)
  , p_image(0)
  , p_XatZ(0)
  , p_YatZ(0)
{
  MSG(DEBUG, "In c-tor GeometryAccess(std::stringstream& ss)"); 
  load_pars_from_stringstream(ss);
}

//--------------

GeometryAccess::~GeometryAccess()
{
  MSG(DEBUG, "In d-tor ~GeometryAccess");
  if(p_iX)    delete [] p_iX;
  if(p_iY)    delete [] p_iY;
  if(p_image) delete [] p_image;
  if(p_XatZ)  delete [] p_XatZ;
  if(p_YatZ)  delete [] p_YatZ;
}

//-------------------

void GeometryAccess::load_pars_from_stringstream(std::stringstream& ss)
{
  m_dict_of_comments.clear();
  v_list_of_geos.clear();
  v_list_of_geos.reserve(100);

  std::string line;
  //while (ss >> line) { // works for std::ifstream
  while (std::getline(ss, line, '\n')) {    
    if(m_pbits & 256) std::cout << line << '\n'; // << " length = " << line.size() << '\n';
    if(line.empty()) continue;                   // discard empty lines
    if(line[0] == '#') {                         // process line of comments 
       add_comment_to_dict(line); 
       continue;
    }
    // make geometry object and add it in the list
    v_list_of_geos.push_back(parse_line(line));    
  }

  set_relations();
}

//-------------------

void GeometryAccess::load_pars_from_string(const std::string& s)
{
  std::stringstream ss(s);
  load_pars_from_stringstream(ss);
}

//-------------------

void GeometryAccess::load_pars_from_file(const std::string& fname)
{
  if(! fname.empty()) m_path = fname;
  if(m_path.empty())  MSG(DEBUG, "geometry file name is not specified, object is not initialized");

  std::stringstream ss;
  //geometry::file_to_stringstream(m_path, ss);
  file_to_stringstream(m_path, ss);
  //cout << "string:\n" << ss.str() <<  "\n";

  load_pars_from_stringstream(ss);
}

//-------------------
//-------------------
//-------------------
//-------------------

void GeometryAccess::save_pars_in_file(const std::string& path)
{
  MSG(DEBUG, "Save pars in file " << path.c_str());

  std::stringstream ss;

  // Save comments
  std::map<int, std::string>::iterator iter;
  for (iter = m_dict_of_comments.begin(); iter != m_dict_of_comments.end(); ++iter) {
    //ss << "# " << std::setw(10) << std::left << iter->first;
    ss << "# " << iter->second << '\n';
  }

  ss << '\n';

  // Save data
  for(std::vector<GeometryAccess::pGO>::iterator it  = v_list_of_geos.begin(); 
                                                 it != v_list_of_geos.end(); ++it) {
    if( (*it)->get_parent_name().empty() ) continue;
    ss << (*it)->str_data() << '\n';
  }

  std::ofstream out(path.c_str()); 
  out << ss.str();
  out.close();

  if(m_pbits & 64) std::cout << ss.str();
}

//-------------------

void GeometryAccess::add_comment_to_dict(const std::string& line)
{ 
  std::size_t p1 = line.find_first_not_of("#");
  std::size_t p2 = line.find_first_not_of(" ", p1);
  //std::size_t p2 = line.find_first_of(" ", p1);

  //std::cout << "  p1:" << p1 << "  p2:" << p2 << '\n';
  //if (p1 == std::string::npos) ...
  //std::cout << "comment: " << line << '\n'; 
  //std::cout << "   split line: [" << beginline << "] = " << endline << '\n'; 

  if (p1 == std::string::npos) return; // available # but missing keyword

  int ind = m_dict_of_comments.size();

  if (p2 == std::string::npos) { // keyword is available but comment is missing
    m_dict_of_comments[ind] = std::string();
    return;
  }

  std::string endline(line, p2);
  m_dict_of_comments[ind] = endline;
}

//-------------------

/*
void GeometryAccess::add_comment_to_dict(const std::string& line)
{ 
  std::size_t p1 = line.find_first_not_of("# ");
  std::size_t p2 = line.find_first_of(" ", p1);
  std::size_t p3 = line.find_first_not_of(" ", p2);

  //std::cout << "  p1:" << p1 << "  p2:" << p2 << "  p3:" << p3 << '\n';
  //if (p1 == std::string::npos) ...
  //std::cout << "comment: " << line << '\n'; 
  //std::cout << "   split line: [" << beginline << "] = " << endline << '\n'; 

  if (p1 == std::string::npos) return; // available # but missing keyword
  if (p2 == std::string::npos) return;

  std::string beginline(line, p1, p2-p1);

  if (p3 == std::string::npos) { // keyword is available but comment is missing
    m_dict_of_comments[beginline] = std::string();
    return;
  }

  std::string endline(line, p3);
  m_dict_of_comments[beginline] = endline;
}
*/

//-------------------

GeometryAccess::pGO GeometryAccess::parse_line(const std::string& line)
{
  std::string   pname;
  segindex_t      pindex;
  std::string   oname;
  segindex_t      oindex;
  pixel_coord_t x0;
  pixel_coord_t y0;
  pixel_coord_t z0;
  angle_t       rot_z;
  angle_t       rot_y;
  angle_t       rot_x;                  
  angle_t       tilt_z;
  angle_t       tilt_y;
  angle_t       tilt_x; 

  std::stringstream ss(line);

  if(ss >> pname >> pindex >> oname >> oindex >> x0 >> y0 >> z0 
        >> rot_z >> rot_y >> rot_x >> tilt_z >> tilt_y >> tilt_x) {
      GeometryAccess::pGO shp(new GeometryObject (pname,
                              		     pindex,
                              		     oname,
                              		     oindex,
                              		     x0,
                              		     y0,
                              	  	     z0,
                              		     rot_z,
                              		     rot_y,
                              		     rot_x,                  
                              		     tilt_z,
                              		     tilt_y,
                              		     tilt_x 
		                             ));
      if(m_pbits & 256) shp->print_geo();
      return shp;
  }
  else {
      std::string msg = "parse_line(...) can't parse line: " + line;
      //std::cout << msg;
      MSG(ERROR, msg);
      return GeometryAccess::pGO();
  }
}

//-------------------

GeometryAccess::pGO GeometryAccess::find_parent(const GeometryAccess::pGO& geobj)
{
  for(std::vector<GeometryAccess::pGO>::iterator it  = v_list_of_geos.begin(); 
                                   it != v_list_of_geos.end(); ++it) {
    if(*it == geobj) continue; // skip geobj themself
    if(   (*it)->get_geo_index() == geobj->get_parent_index()
       && (*it)->get_geo_name()  == geobj->get_parent_name()) {
      return (*it);
    }
  }

  //The name of parent object is not found among geos in the v_list_of_geos
  // add top parent object to the list

  if(m_pbits & 256) std::cout << "  GeometryAccess::find_parent(...): parent is not found..."
                              << geobj->get_parent_name() << " idx:" << geobj->get_parent_index() << '\n';

  if( ! geobj->get_parent_name().empty() ) { // skip top parent itself

    if(m_pbits & 256) std::cout << "  create one with name:" << geobj->get_parent_name() 
                                << " idx:" << geobj->get_parent_index() << '\n';

    GeometryAccess::pGO shp_top_parent(new GeometryObject (std::string(),
                                	                   0,
                            		                   geobj->get_parent_name(),
                            		                   geobj->get_parent_index()));

    v_list_of_geos.push_back(shp_top_parent);
    return shp_top_parent;		  
  }

  if(m_pbits & 256) std::cout << "  return empty pGO() for the very top parent\n";
  return GeometryAccess::pGO(); // for top parent itself
}

//-------------------

void GeometryAccess::set_relations()
{
  MSG(DEBUG, "Begin set_relations(): size of the list:" << v_list_of_geos.size());
  for(std::vector<GeometryAccess::pGO>::iterator it  = v_list_of_geos.begin(); 
                                                 it != v_list_of_geos.end(); ++it) {

    GeometryAccess::pGO shp_parent = find_parent(*it);
    //std::cout << "set_relations(): found parent name:" << shp_parent->get_parent_name()<<'\n';

    if( shp_parent == GeometryAccess::pGO() ) continue; // skip parent of the top object
    
    (*it)->set_parent(shp_parent);
    shp_parent->add_child(*it);

    if(m_pbits & 16) 
      std::cout << "\n  geo:"     << std::setw(10) << (*it) -> get_geo_name()
                << " : "                           << (*it) -> get_geo_index()
                << " has parent:" << std::setw(10) << shp_parent -> get_geo_name()
                << " : "                           << shp_parent -> get_geo_index()
                << '\n';
  }
}

//-------------------

GeometryAccess::pGO GeometryAccess::get_geo(const std::string& oname, const segindex_t& oindex)
{
  for(std::vector<GeometryAccess::pGO>::iterator it  = v_list_of_geos.begin(); 
                                                 it != v_list_of_geos.end(); ++it) {
    if(   (*it)->get_geo_index() == oindex
       && (*it)->get_geo_name()  == oname ) 
          return (*it);
  }
  return GeometryAccess::pGO(); // None
}

//-------------------

GeometryAccess::pGO GeometryAccess::get_top_geo()
{
  return v_list_of_geos.back();
}

//-------------------

void
GeometryAccess::get_pixel_coords(const pixel_coord_t*& X, 
                                 const pixel_coord_t*& Y, 
                                 const pixel_coord_t*& Z, 
				 gsize_t& size,
                                 const std::string& oname, 
                                 const segindex_t& oindex,
                                 const bool do_tilt,
                                 const bool do_eval)
{
  GeometryAccess::pGO geo = (oname.empty()) ? get_top_geo() : get_geo(oname, oindex);
  if(m_pbits & 32) {
    std::stringstream ss; ss << "get_pixel_coords(...) for geo:\n" << geo -> string_geo_children()
                             << "  do_tilt: " << do_tilt << "  do_eval: " << do_eval; 
    MSG(INFO, ss.str());
  }
  geo -> get_pixel_coords(X, Y, Z, size, do_tilt, do_eval);
}

//-------------------

void
GeometryAccess::get_pixel_xy_at_z(const pixel_coord_t*& XatZ, 
                                  const pixel_coord_t*& YatZ, 
				  gsize_t& size,
                                  const pixel_coord_t& Zplain, 
                                  const std::string& oname, 
                                  const segindex_t& oindex)
{
  const pixel_coord_t* X;
  const pixel_coord_t* Y;
  const pixel_coord_t* Z;
  //gsize_t  size;
  const bool do_tilt=true;
  const bool do_eval=true;
  get_pixel_coords(X,Y,Z,size,oname,oindex,do_tilt,do_eval);

  if (!size) {
    MSG(ERROR, "get_pixel_coords returns ZERO size coordinate array...");
    abort(); 
  }

  if (p_XatZ) delete [] p_XatZ;
  if (p_YatZ) delete [] p_YatZ;

  p_XatZ = new pixel_coord_t[size];
  p_YatZ = new pixel_coord_t[size];

  // find Z0 as average Z if Zplain is not specified
  pixel_coord_t Z0 = 0;
  if (Zplain) Z0 = Zplain;
  else {
    for(gsize_t i=0; i<size; ++i) Z0 += Z[i]; 
    Z0 = Z0/size;
  }

  if(fabs(Z0) < 1000) {  
    XatZ = X;
    YatZ = Y;
    return;
  }  

  pixel_coord_t R;
  for(gsize_t i=0; i<size; ++i) {
    if (Z[i]) {
      R = Z0/Z[i];
      p_XatZ[i] = X[i]*R;
      p_YatZ[i] = Y[i]*R;
    } else {
      p_XatZ[i] = X[i];
      p_YatZ[i] = Y[i];
    }
  }
  XatZ = p_XatZ;
  YatZ = p_YatZ;
}

//-------------------

void
GeometryAccess::get_pixel_areas(const pixel_area_t*& A, 
				gsize_t& size,
                                const std::string& oname, 
                                const segindex_t& oindex)
{
  GeometryAccess::pGO geo = (oname.empty()) ? get_top_geo() : get_geo(oname, oindex);
  if(m_pbits & 32) {
    std::string msg = "get_pixel_areas(...) for geo:\n" + geo -> string_geo_children();
    MSG(INFO, msg);
  }
  geo -> get_pixel_areas(A, size);
}

//-------------------

void
GeometryAccess::get_pixel_mask(const pixel_mask_t*& mask,
			       gsize_t& size,
			       const std::string& oname,
			       const segindex_t& oindex,
			       const bitword_t& mbits)
{
  //cout << "GeometryAccess::get_pixel_mask(): mbits =" << mbits << '\n';   

  GeometryAccess::pGO geo = (oname.empty()) ? get_top_geo() : get_geo(oname, oindex);
  if(m_pbits & 32) {
    std::string msg = "get_pixel_areas(...) for geo:\n" + geo -> string_geo_children();
    MSG(INFO, msg);
  }
  geo -> get_pixel_mask(mask, size, mbits);
}

//-------------------

pixel_coord_t
GeometryAccess::get_pixel_scale_size(const std::string& oname,
                                     const segindex_t& oindex)
{
  GeometryAccess::pGO geo = (oname.empty()) ? get_top_geo() : get_geo(oname, oindex);
  return geo -> get_pixel_scale_size();
}

//-------------------

void
GeometryAccess::set_geo_pars(const std::string& oname, 
		             const segindex_t& oindex,
                             const pixel_coord_t& x0,
                             const pixel_coord_t& y0,
                             const pixel_coord_t& z0,
                             const angle_t      & rot_z,
                             const angle_t      & rot_y,
                             const angle_t      & rot_x,                  
                             const angle_t      & tilt_z,
                             const angle_t      & tilt_y,
                             const angle_t      & tilt_x 
		             )
{
  GeometryAccess::pGO geo = (oname.empty()) ? get_top_geo() : get_geo(oname, oindex);
  geo -> set_geo_pars(x0, y0, z0, rot_z, rot_y, rot_x, tilt_z, tilt_y, tilt_x);
}

//-------------------

void
GeometryAccess::move_geo(const std::string& oname, 
		         const segindex_t& oindex,
                         const pixel_coord_t& dx,
                         const pixel_coord_t& dy,
                         const pixel_coord_t& dz
			 )
{
  GeometryAccess::pGO geo = (oname.empty()) ? get_top_geo() : get_geo(oname, oindex);
  geo -> move_geo(dx, dy, dz);
}

//-------------------

void
GeometryAccess::tilt_geo(const std::string& oname, 
		         const segindex_t& oindex,
                         const pixel_coord_t& dt_x,
                         const pixel_coord_t& dt_y,
                         const pixel_coord_t& dt_z
			 )
{
  GeometryAccess::pGO geo = (oname.empty()) ? get_top_geo() : get_geo(oname, oindex);
  geo -> tilt_geo(dt_x, dt_y, dt_z);
}

//-------------------

void GeometryAccess::print_list_of_geos()
{
  std::stringstream ss; ss << "print_list_of_geos():";
  if( v_list_of_geos.empty() ) ss << "List of geos is empty...";
  for(std::vector<GeometryAccess::pGO>::iterator it  = v_list_of_geos.begin(); 
                                                 it != v_list_of_geos.end(); ++it) {
    ss << '\n' << (*it)->string_geo();
  }
  //std::cout << ss.str();
  MSG(INFO, ss.str());
}

//-------------------

void GeometryAccess::print_list_of_geos_children()
{
  std::stringstream ss; ss << "print_list_of_geos_children(): ";
  if( v_list_of_geos.empty() ) ss << "List of geos is empty...";

  for(std::vector<GeometryAccess::pGO>::iterator it  = v_list_of_geos.begin(); 
                                                 it != v_list_of_geos.end(); ++it) {
    ss << '\n' << (*it)->string_geo_children();
  }
  //std::cout << ss.str() << '\n';
  MSG(INFO, ss.str());
}

//-------------------

void GeometryAccess::print_comments_from_dict()
{ 
  std::stringstream ss; ss << "print_comments_from_dict():\n"; 

  std::map<int, std::string>::iterator iter;

  for (iter = m_dict_of_comments.begin(); iter != m_dict_of_comments.end(); ++iter) {
    ss << "  key:" << std::setw(4) << std::left << iter->first;
    ss << "  val:" << iter->second << '\n';
  }
  //std::cout << ss.str();
  MSG(INFO, ss.str());
}
//-------------------

void
GeometryAccess::print_pixel_coords(const std::string& oname, 
                                   const segindex_t& oindex)
{
  const pixel_coord_t* X;
  const pixel_coord_t* Y;
  const pixel_coord_t* Z;
  gsize_t size;
  const bool do_tilt=true;
  get_pixel_coords(X,Y,Z,size,oname,oindex,do_tilt);

  std::stringstream ss; ss << "print_pixel_coords():\n"
			   << "size=" << size << '\n' << std::fixed << std::setprecision(1);  
  ss << "X: "; for(unsigned i=0; i<10; ++i) ss << std::setw(10) << X[i] << ", "; ss << "...\n";
  ss << "Y: "; for(unsigned i=0; i<10; ++i) ss << std::setw(10) << Y[i] << ", "; ss << "...\n"; 
  ss << "Z: "; for(unsigned i=0; i<10; ++i) ss << std::setw(10) << Z[i] << ", "; ss << "...\n"; 
  //cout << ss.str();
  MSG(INFO, ss.str());
}

//-------------------

void
GeometryAccess::print_geometry_info(const bitword_t& pbits) {
  if(pbits & 1) print_comments_from_dict();
  if(pbits & 2) print_list_of_geos();
  if(pbits & 4) print_list_of_geos_children();
  if(pbits & 8) print_pixel_coords();
}

//-------------------

void
GeometryAccess::get_pixel_coord_indexes(const pixel_idx_t *& iX, 
                                        const pixel_idx_t *& iY, 
				        gsize_t& size,
                                        const std::string& oname, 
					const segindex_t& oindex, 
                                        const pixel_coord_t& pix_scale_size_um, 
                                        const int* xy0_off_pix,
                                        const bool do_tilt)
{
  const pixel_coord_t* X;
  const pixel_coord_t* Y;
  const pixel_coord_t* Z;

  get_pixel_coords(X,Y,Z,size,oname,oindex,do_tilt);
  
  pixel_coord_t pix_size = (pix_scale_size_um) ? pix_scale_size_um : get_pixel_scale_size(oname, oindex);

  if (p_iX) delete [] p_iX;
  if (p_iY) delete [] p_iY;

  p_iX = new pixel_idx_t[size];
  p_iY = new pixel_idx_t[size];

  if (xy0_off_pix) {
    // Offset in pix -> um
    pixel_coord_t x_off_um = xy0_off_pix[0] * pix_size;
    pixel_coord_t y_off_um = xy0_off_pix[1] * pix_size;
    // Protection against wrong offset bringing negative indexes
    pixel_coord_t x_min=0; for(gsize_t i=0; i<size; ++i) {if (X[i] + x_off_um < x_min) x_min = X[i] + x_off_um;} x_off_um -= x_min - pix_size/2;
    pixel_coord_t y_min=0; for(gsize_t i=0; i<size; ++i) {if (Y[i] + y_off_um < y_min) y_min = Y[i] + y_off_um;} y_off_um -= y_min - pix_size/2;

    for(gsize_t i=0; i<size; ++i) { 
      p_iX[i] = (pixel_idx_t)((X[i] + x_off_um) / pix_size);
      p_iY[i] = (pixel_idx_t)((Y[i] + y_off_um) / pix_size);
    }
  } 
  else {
    // Find coordinate min values
    pixel_coord_t x_min=X[0]; for(gsize_t i=0; i<size; ++i) {if (X[i] < x_min) x_min = X[i];} x_min -= pix_size/2;
    pixel_coord_t y_min=Y[0]; for(gsize_t i=0; i<size; ++i) {if (Y[i] < y_min) y_min = Y[i];} y_min -= pix_size/2;
    for(gsize_t i=0; i<size; ++i) { 
      p_iX[i] = (pixel_idx_t)((X[i] - x_min) / pix_size);
      p_iY[i] = (pixel_idx_t)((Y[i] - y_min) / pix_size);
    }
  }

  iX = p_iX;
  iY = p_iY;
}

//-------------------

void
GeometryAccess::get_pixel_xy_inds_at_z(const pixel_idx_t *& iX, 
                                       const pixel_idx_t *& iY, 
				       gsize_t& size,
                                       const pixel_coord_t& Zplain, 
                                       const std::string& oname, 
				       const segindex_t& oindex, 
                                       const pixel_coord_t& pix_scale_size_um, 
                                       const int* xy0_off_pix)
{
  const pixel_coord_t* X;
  const pixel_coord_t* Y;

  get_pixel_xy_at_z(X,Y,size,Zplain,oname,oindex);
  
  pixel_coord_t pix_size = (pix_scale_size_um) ? pix_scale_size_um : get_pixel_scale_size(oname, oindex);

  if (p_iX) delete [] p_iX;
  if (p_iY) delete [] p_iY;

  p_iX = new pixel_idx_t[size];
  p_iY = new pixel_idx_t[size];

  if (xy0_off_pix) {
    // Offset in pix -> um
    pixel_coord_t x_off_um = xy0_off_pix[0] * pix_size;
    pixel_coord_t y_off_um = xy0_off_pix[1] * pix_size;
    // Protection against wrong offset bringing negative indexes
    pixel_coord_t x_min=0; for(gsize_t i=0; i<size; ++i) {if (X[i] + x_off_um < x_min) x_min = X[i] + x_off_um;} x_off_um -= x_min - pix_size/2;
    pixel_coord_t y_min=0; for(gsize_t i=0; i<size; ++i) {if (Y[i] + y_off_um < y_min) y_min = Y[i] + y_off_um;} y_off_um -= y_min - pix_size/2;

    for(gsize_t i=0; i<size; ++i) { 
      p_iX[i] = (pixel_idx_t)((X[i] + x_off_um) / pix_size);
      p_iY[i] = (pixel_idx_t)((Y[i] + y_off_um) / pix_size);
    }
  } 
  else {
    // Find coordinate min values
    pixel_coord_t x_min=X[0]; for(gsize_t i=0; i<size; ++i) {if (X[i] < x_min) x_min = X[i];} x_min -= pix_size/2;
    pixel_coord_t y_min=Y[0]; for(gsize_t i=0; i<size; ++i) {if (Y[i] < y_min) y_min = Y[i];} y_min -= pix_size/2;
    for(gsize_t i=0; i<size; ++i) { 
      p_iX[i] = (pixel_idx_t)((X[i] - x_min) / pix_size);
      p_iY[i] = (pixel_idx_t)((Y[i] - y_min) / pix_size);
    }
  }

  iX = p_iX;
  iY = p_iY;
}

//-------------------
//-------------------

NDArray<image_t> &
GeometryAccess::ref_img_from_pixel_arrays(const pixel_idx_t*& iX, 
                                          const pixel_idx_t*& iY, 
                                          const double*    W,
                                          const gsize_t&  size)
{
    pixel_idx_t ix_max=iX[0]; for(gsize_t i=0; i<size; ++i) {if (iX[i] > ix_max) ix_max = iX[i];} ix_max++;
    pixel_idx_t iy_max=iY[0]; for(gsize_t i=0; i<size; ++i) {if (iY[i] > iy_max) iy_max = iY[i];} iy_max++;

    shape_t sh[2] = {ix_max, iy_max};

    if(p_image) delete [] p_image;
    p_image = new NDArray<image_t>(sh, 2);
    NDArray<image_t>& img = *p_image;

    std::fill_n(img.data(), int(img.size()), image_t(0));

    if (W) for(gsize_t i=0; i<size; ++i) img(iX[i],iY[i]) = (image_t) W[i];
    else   for(gsize_t i=0; i<size; ++i) img(iX[i],iY[i]) = 1;
    return *p_image;
}

//-------------------
//-------------------
//-- Static Methods--
//-------------------
//-------------------

NDArray<image_t>
GeometryAccess::img_from_pixel_arrays(const pixel_idx_t*& iX, 
                                      const pixel_idx_t*& iY, 
                                      const double*    W,
                                      const gsize_t&  size)
{
    pixel_idx_t ix_max=iX[0]; for(gsize_t i=0; i<size; ++i) {if (iX[i] > ix_max) ix_max = iX[i];} ix_max++;
    pixel_idx_t iy_max=iY[0]; for(gsize_t i=0; i<size; ++i) {if (iY[i] > iy_max) iy_max = iY[i];} iy_max++;

    shape_t sh[2] = {ix_max, iy_max};
    NDArray<image_t>& img = *(new NDArray<image_t>(sh, 2));
    std::fill_n(img.data(), int(img.size()), image_t(0));

    if (W) for(gsize_t i=0; i<size; ++i) img(iX[i],iY[i]) = (image_t) W[i];
    else   for(gsize_t i=0; i<size; ++i) img(iX[i],iY[i]) = 1;
    return img;
}

//-------------------

NDArray<const pixel_coord_t>*
GeometryAccess::get_pixel_coords(const AXIS axis)
{
    const pixel_coord_t* X;
    const pixel_coord_t* Y;
    const pixel_coord_t* Z;
    gsize_t size;
    get_pixel_coords(X,Y,Z,size, std::string(), 0, true); //,oname,oindex,do_tilt

    //cout << "XXX get_pixel_coords size: " << size << '\n';
    shape_t sh[1] = {size};

    switch(axis) {
      default         : 
      case AXIS_X : return new NDArray<const pixel_coord_t>(&sh[0], 1, X);
      case AXIS_Y : return new NDArray<const pixel_coord_t>(&sh[0], 1, Y);
      case AXIS_Z : return new NDArray<const pixel_coord_t>(&sh[0], 1, Z);
    }
}
//-------------------

NDArray<const pixel_coord_t>*
GeometryAccess::get_pixel_coords_at_z(const pixel_coord_t Zplane, const AXIS axis) {
    const pixel_coord_t* X;
    const pixel_coord_t* Y;
    gsize_t size;
    get_pixel_xy_at_z(X,Y,size, Zplane, std::string(), 0);

    //cout << "XXX get_pixel_coords_at_z size: " << size << '\n';
    shape_t sh[1] = {size,};

    switch(axis) {
      default         : 
      case AXIS_X : return new NDArray<const pixel_coord_t>(sh, 1, X);
      case AXIS_Y : return new NDArray<const pixel_coord_t>(sh, 1, Y);
    }
}
//-------------------

NDArray<const pixel_idx_t>*
GeometryAccess::get_pixel_coord_indexes(const AXIS axis,
                                        const pixel_coord_t pix_scale_size_um,
                                        const int* xy0_off_pix)
{
  const pixel_idx_t* iX;
  const pixel_idx_t* iY;
  gsize_t size;

  get_pixel_coord_indexes(iX, iY, size, std::string(), 0, pix_scale_size_um, xy0_off_pix, true);
  //cout << "XXX get_pixel_coord_indexes size: " << size << '\n';
  shape_t sh[1] = {size,};

  switch(axis) {
    default         : 
    case AXIS_X : return new NDArray<const pixel_idx_t>(sh, 1, iX);
    case AXIS_Y : return new NDArray<const pixel_idx_t>(sh, 1, iY);
  }
}

//-------------------

NDArray<const pixel_idx_t>* 
GeometryAccess::get_pixel_inds_at_z(const pixel_coord_t Zplane,
                                    const AXIS axis,
                                    const pixel_coord_t pix_scale_size_um,
                                    const int* xy0_off_pix)
{
  const pixel_idx_t* iX;
  const pixel_idx_t* iY;
  gsize_t size;
  get_pixel_xy_inds_at_z(iX, iY, size, Zplane, std::string(), 0, pix_scale_size_um, xy0_off_pix);
  shape_t sh[1] = {size,};

  switch(axis) {
    default         : 
    case AXIS_X : return new NDArray<const pixel_idx_t>(sh, 1, iX);
    case AXIS_Y : return new NDArray<const pixel_idx_t>(sh, 1, iY);
  }
}

//-------------------

NDArray<const pixel_area_t>*
GeometryAccess::get_pixel_areas()
{
    const pixel_area_t* A;
    gsize_t size;
    get_pixel_areas(A, size, std::string(), 0);
    //cout << "XXX get_pixel_areas: " << size << '\n';
    shape_t sh[1] = {size,};
    return new NDArray<const pixel_area_t>(sh, 1, A);
}

//-------------------

NDArray<const pixel_mask_t>*
GeometryAccess::get_pixel_mask(const bitword_t& mbits)
{
    const pixel_mask_t* mask;
    gsize_t size;
    get_pixel_mask(mask, size, std::string(), 0, mbits);
    //cout << "XXX get_pixel_mask: " << size << '\n';
    shape_t sh[1] = {size,};
    return new NDArray<const pixel_mask_t>(sh, 1, mask);
}

//-------------------

void GeometryAccess::file_to_stringstream(const std::string& fname, std::stringstream& ss)
{
  std::ifstream f(fname.c_str());

  if(f.good()) {MSG(DEBUG, "file_to_stringstream(...): " << fname);}
  else         {MSG(ERROR, "File " << fname << " does not exist");}

  ss << f.rdbuf();
  f.close();
  //return ss.str();
}

//std::ostream& operator<<(std::ostream& out, const CalibFile& cf)

//-------------------

} // namespace geometry

//-------------------
