function [hor_err,ecef_err,ned_err]=compt_err(Lat_d,Lon_d,H,grd,llh_grd)
%     f  = 1/298.257223563;
%     % equatorial radius
%     R =  6378137;
%     % sind/cosd is sin/cos in degrees, if using func in radians, rememeber to covert it. 
%     sinphi = sind(Lat_d); 
%     cosphi = cosd(Lat_d);
%     e2 = f * (2 - f);
%     N  = R / sqrt(1 - e2 * sinphi^2);
%     rho = (N + H) * cosphi;
%     z = (N*(1 - e2) + H) * sinphi;
%     x = rho*cosd(Lon_d);
%     y = rho*sind(Lon_d);
    p = lla2ecef([Lat_d,Lon_d,H]);
    wgs84 = wgs84Ellipsoid('meter');
%     p = [x;y;z]; % Position in ECEF
    [xNorth,yEast,zDown] = ecef2ned(p(1),p(2),p(3),llh_grd(1),llh_grd(2),llh_grd(3),wgs84);
%     sin_lat= sin(llh_grd(1,1));
%     cos_lat= cos(llh_grd(1,1));
%     sin_lng= sin(llh_grd(2,1));
%     cos_lng= cos(llh_grd(2,1));
%     C_e_n = [   -sin_lat*cos_lng -sin_lat*sin_lng   cos_lat; % Farrell eqn. 2.32
%             -sin_lng          cos_lng           0;
%             -cos_lat*cos_lng -cos_lat*sin_lng  -sin_lat];
%     err_pos = grd - p;
%     ned_err = C_e_n*err_pos;
%     hor_err = norm(ned_err(1:2));
%     ecef_err = norm(err_pos);
    ned_err = [xNorth;yEast;zDown];
    hor_err = norm([xNorth,yEast]);
    ecef_err = norm(grd - p');
end