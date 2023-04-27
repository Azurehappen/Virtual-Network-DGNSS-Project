#include "us_tec_iono_corr_computer.h"

/****************************************************************/
UsTecIonoCorrComputer::UsTecIonoCorrComputer(std::vector<std::vector<int>>  USTEC_map_file, std::vector<double>  user_location)
:TEC_grid_map(std::move(USTEC_map_file)),
user_pos_ecef(std::move(user_location)),
pi(3.14159265358980),
Earth_radius(6378136.3000),
iono_height(350000),
OMEGA_ie(7.29211e-5),
semi_major_axis_len(6378137.0),
semi_minor_axis_len(6356752.31424518),
flat_e(1.0/298.257223563),
ecc_e(sqrt((pow(semi_major_axis_len,2)-pow(semi_minor_axis_len,2))/pow(semi_major_axis_len,2)))
{
    ECEF2LatLon();
    Earth2LevelRotation();
}
/*****************************************************/
UsTecIonoCorrComputer::~UsTecIonoCorrComputer() = default;

/***************************************************/
bool UsTecIonoCorrComputer::ParseUSTECFile()
{
  std::ifstream USTECdata(USTEC_data_path);
  std::string line;
    if(USTECdata.is_open()){
        //Parsing the header files
        size_t line_num=1;
        while( line_num<=18 && getline(USTECdata,line)){
            line_num++;
                   }
        // Parsing line 19
        //Parses the longitude bins
        getline(USTECdata,line);
      std::cout<<"Data Parsing starts:"<<std::endl;
      std::stringstream line_stream(line);
      std::string data_val;
        TEC_grid_map.push_back(std::vector<int>());
      std::vector<int> temp_row;
        while(line_stream.good()){
            getline(line_stream,data_val,' ');
            if(data_val.length()>1)
                temp_row.push_back(stoi(data_val));
        }
        TEC_grid_map[0]=temp_row;
        //Parsing line 20 to 70
        size_t row_num=1;
        while(line_num<70) {
            getline(USTECdata,line);
          std::stringstream line_stream(line);
          std::string data_val;
            TEC_grid_map.push_back(std::vector<int>());
          std::vector<int> temp_row;
            while (line_stream.good()) {
                getline(line_stream, data_val, ' ');
                if (data_val.length() > 1)
                    temp_row.push_back(stoi(data_val));
            }
            TEC_grid_map[row_num] = temp_row;
            row_num++;
            line_num++;
        }
        USTECdata.close();
    }
    else return false;
    return true;
}
/******************************************/
void UsTecIonoCorrComputer::ECEF2LatLon(){
    double_t h = 0;
    double N = semi_major_axis_len;
    double p = sqrt(pow(user_pos_ecef[0],2)+pow(user_pos_ecef[1],2));
    double z = user_pos_ecef[2];
    double lambda = 0.0;
    for(int iter=0;iter<=10; iter++)
    {
        double s_lambda = z/(N*(1.0-pow(ecc_e,2))+h);
        if(s_lambda>1) s_lambda=1;
        if(s_lambda<-1) s_lambda=-1;
        lambda = atan2((z+pow(ecc_e,2)*N*s_lambda),p);
        N = semi_major_axis_len/(sqrt(1.0 - pow(ecc_e*s_lambda,2)));
        h = p/cos(lambda)-N;
    }
    user_latitude = lambda;
    user_longitude = atan2(user_pos_ecef[1],user_pos_ecef[0]);
    user_height = h;
    return ;
}

void UsTecIonoCorrComputer::Earth2LevelRotation(){
   double slat = sin(user_latitude)  ;
   double clat = cos(user_latitude);
   double slon = sin(user_longitude);
   double clon = cos(user_longitude);
  std::vector<std::vector<double>> temp(3,std::vector<double>(3,0.0));
   temp[0][0]=-slat*clon;
   temp[0][1]=-slat*slon;
   temp[0][2]=clat;
   temp[1][0]=-slon;
   temp[1][1]=clon;
   temp[1][2]=0.0;
   temp[2][0]=-clat*clon;
   temp[2][1]=-clat*slon;
   temp[2][2]=-slat;
   R_e2l=temp;

    return ;
}
std::vector<double>
UsTecIonoCorrComputer::Matrix_vector_multiplication(const std::vector<std::vector<double>>& A, const std::vector<double>& B){
    int row = A.size();
    int col = A[0].size();
  std::vector<double> C(row,0.0);
    for(int i=0;i<row;i++)
    {
        double sum = 0.0;
        for(int j=0; j<col; j++)
            sum+=A[i][j]*B[j];
        C[i]=sum;
    }
    return C;
}

std::vector<double>
UsTecIonoCorrComputer::ElevationAzimuthComputation(const std::vector<double>& sat_pos){
    // geodetic position in semi-circles
    //double phi_u = user_latitude/pi;
    //double lambda_u = user_longitude/pi;

    //user and satellite positions in local level coordinate system
  std::vector<double> r_u_level = Matrix_vector_multiplication(R_e2l,user_pos_ecef);
  std::vector<double> r_s_level = Matrix_vector_multiplication(R_e2l,sat_pos);
  std::vector<double> dr (3,0.0);
    dr[0]= r_s_level[0]-r_u_level[0];
    dr[1]= r_s_level[1]-r_u_level[1];
    dr[2]= r_s_level[2]-r_u_level[2];

    double elevation = atan2(-dr[2],sqrt(pow(dr[0],2) + pow(dr[1],2) ));
    double azimuth = atan2(dr[1],dr[0]);
    return {elevation,azimuth} ;
}
std::vector<double> UsTecIonoCorrComputer::PiercePointCalculation (const double& elevation, const double& azimuth){
    // Prol-2017
    double psi_r = pi/2 - elevation - asin(Earth_radius*cos(elevation)/(Earth_radius+iono_height));
    double temp = sin(user_latitude)*cos(psi_r)+cos(user_latitude)*sin(psi_r)*cos(azimuth);
    double phi_ip = asin(temp);
    double lambda_ip = user_longitude + asin(sin(psi_r)*sin(azimuth)/cos(phi_ip));
    double pierce_latitude = phi_ip*180/pi;
    double pierce_longitude = lambda_ip*180/pi;
    return {pierce_latitude,pierce_longitude};
}
int UsTecIonoCorrComputer::VerticalTECComputation(const std::vector<double>& sat_pos){
    // variables for different users
    double pierce_latitude;
    double pierce_longitude;

    // compute elevation and azimuth angle
  std::vector<double> geometry = ElevationAzimuthComputation(sat_pos);
    elevation = geometry[0];
    azimuth = geometry[1];
    if (elevation<ELEVMASK){
        //cout<<"This satellite is out of view: "<<endl;
        vertical_TEC=-1;
        return -1;
    }
    //cout<< "elevation angle: "<<elevation <<endl;
    //cout<<"azimuth angle: "<<azimuth<<endl;

    // compute the pierce points
  std::vector<double > pierce_point = PiercePointCalculation(elevation,azimuth);
    pierce_latitude = pierce_point[0];
    pierce_longitude = pierce_point[1];

    //cout<<"pierce point: "<< pierce_latitude<<" "<<pierce_longitude<<endl;
    //if(pierce_latitude<=min_lat) cout<<"cond 1 ";
    //if(pierce_latitude>=max_lat)cout<<"cond 2";
    //if(pierce_longitude<=min_lon)cout<<"cond 3";
    //if(pierce_longitude>=max_lon)cout<<"cond 4";
    if(pierce_latitude<=min_lat || pierce_latitude>=max_lat || pierce_longitude<=min_lon || pierce_longitude>=max_lon)
    {
        //cout<<"This satellite is out of view: "<<endl;
        vertical_TEC=-1;
        return -1;
    }

    // find out the closest point of longitude in TEC map
    int idx=1;
    while((TEC_grid_map[0][idx])/10<pierce_longitude)
        idx++;
  std::vector<int> col_idx;
    col_idx.push_back(idx-1);
    col_idx.push_back(idx);
    //cout<<col_idx[0]<<" "<<col_idx[1]<<endl;
    // find out the closest point of latitude in TEC map
    idx=1;
    while((TEC_grid_map[idx][0])/10<pierce_latitude)
        idx++;
  std::vector<double> row_idx;
    row_idx.push_back(idx-1);
    row_idx.push_back(idx);

    //cout<<row_idx[0]<<" "<<row_idx[1]<<endl;
    /*
    if(row_idx[0]<1 || row_idx[1]<1 || row_idx[0]>=TEC_grid_map.size() || row_idx[1]>=TEC_grid_map.size() )
    {
        cout<<"This satellite is out of view: "<<endl;
        vertical_TEC=-1;
        return vertical_TEC;
    }
    if(col_idx[0]<1 || col_idx[1]<1 || col_idx[0]>=TEC_grid_map[0].size() || col_idx[1]>=TEC_grid_map[0].size() )
    {
        cout<<"This satellite is out of view: "<<endl;
        vertical_TEC=-1;
        return vertical_TEC;
    }
    */

    // neighboring points
  std::vector<std::pair<double,double>> neighboring_points;
    neighboring_points.push_back(std::make_pair(TEC_grid_map[row_idx[0]][0]/10,TEC_grid_map[0][col_idx[0]]/10));
    neighboring_points.push_back(std::make_pair(TEC_grid_map[row_idx[1]][0]/10,TEC_grid_map[0][col_idx[0]]/10));
    neighboring_points.push_back(std::make_pair(TEC_grid_map[row_idx[0]][0]/10,TEC_grid_map[0][col_idx[1]]/10));
    neighboring_points.push_back(std::make_pair(TEC_grid_map[row_idx[1]][0]/10,TEC_grid_map[0][col_idx[1]]/10));

    // compute distance of the grid points from the pierce points
  std::vector<double> distance_vector(4,0.0);
    distance_vector[0]=sqrt(pow((pierce_latitude-neighboring_points[0].first),2)+pow((pierce_longitude-neighboring_points[0].second),2));
    distance_vector[1]=sqrt(pow((pierce_latitude-neighboring_points[1].first),2)+pow((pierce_longitude-neighboring_points[1].second),2));
    distance_vector[2]=sqrt(pow((pierce_latitude-neighboring_points[2].first),2)+pow((pierce_longitude-neighboring_points[2].second),2));
    distance_vector[3]=sqrt(pow((pierce_latitude-neighboring_points[3].first),2)+pow((pierce_longitude-neighboring_points[3].second),2));

    // inverse weight vector
  std::vector<double> weight_vector(4,0.0);
    double weight_sum =0.0;

    for(int i=0; i<4;i++)
    {
        weight_vector[i]=(1/distance_vector[i]);
        weight_sum+=weight_vector[i];
    }
    for(int i=0; i<4;i++)
        weight_vector[i]=weight_vector[i]/weight_sum;

    vertical_TEC=TEC_grid_map[row_idx[0]][col_idx[0]]*weight_vector[0]
            +TEC_grid_map[row_idx[1]][col_idx[0]]*weight_vector[1]
            +TEC_grid_map[row_idx[0]][col_idx[1]]*weight_vector[2]
            +TEC_grid_map[row_idx[1]][col_idx[1]]*weight_vector[3];
    vertical_TEC=vertical_TEC/10;

    return 0;
}
double UsTecIonoCorrComputer::SlantTECComputation (const double& V_TEC){
    double mp_temp = Earth_radius*cos(elevation)/(Earth_radius+iono_height);
    double mp_fact = sqrt(1-pow(mp_temp,2));
    mp_fact = 1/mp_fact;
    return V_TEC*mp_fact;

}
double UsTecIonoCorrComputer::TEC2IonoDelayComputation(const double &S_TEC, const double &frequency) {
    return (40.3/pow(frequency,2))*S_TEC*1e16;
}

void UsTecIonoCorrComputer::GetLatLonHeight(double& lat, double& lon, double& h)
{
    lat = user_latitude;
    lon = user_longitude;
    h = user_height;
    return ;
}

double UsTecIonoCorrComputer::GetElevation()
{
    return elevation;
}

double UsTecIonoCorrComputer::GetVTEC()
{
    return vertical_TEC;
}