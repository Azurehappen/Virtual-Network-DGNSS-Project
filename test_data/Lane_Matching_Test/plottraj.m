warning off
RTK = readtable('RTK.csv');
STD = readtable('SPS.csv');
VND = readtable('VND.csv');
RTK_lat_all = RTK.Lat;
RTK_lon_all = RTK.Lon;
RTK_t = RTK.UTC;
RTK_t = datetime(RTK_t,'InputFormat','HH:mm:ss.SSS MM/dd/yyyy');
% RTK_t = RTK_t - hours(7);
STD_lat_all = STD.Lat;
STD_lon_all = STD.Lon;
STD_t = STD.UTC;
STD_t = datetime(STD_t,'InputFormat','HH:mm:ss.SSS MM/dd/yyyy');
% STD_t = STD_t - hours(7);
VND_lat_all = VND.Lat;
VND_lon_all = VND.Lon;
VND_t = VND.UTC;
VND_t = datetime(VND_t,'InputFormat','HH:mm:ss.SSS MM/dd/yyyy');
% VND_t = VND_t - hours(7);
lat0 = 34.000916;
lon0 = -117.338011;
wgs84 = wgs84Ellipsoid('meter');

% Match the time to RTK
[Lia,Locb] = ismember(STD_t,RTK_t);
idx = find(Lia == 1);
idx_r = Locb(idx);
STD_t = STD_t(idx);
STD_lat_all = STD_lat_all(idx);
STD_lon_all = STD_lon_all(idx);
RTK_t = RTK_t(idx_r);
RTK_lat_all = RTK_lat_all(idx_r);
RTK_lon_all = RTK_lon_all(idx_r);

[Lia,Locb] = ismember(VND_t,RTK_t);
idx = find(Lia == 1);
idx_r = Locb(idx);
VND_t = VND_t(idx);
VND_lat_all = VND_lat_all(idx);
VND_lon_all = VND_lon_all(idx);
RTK_t = RTK_t(idx_r);
RTK_lat_all = RTK_lat_all(idx_r);
RTK_lon_all = RTK_lon_all(idx_r);
STD_t = STD_t(idx_r);
STD_lat_all = STD_lat_all(idx_r);
STD_lon_all = STD_lon_all(idx_r);
%% Generate road side
p1 = 0.001617331;
E = -50:0.1:240;
N_down = p1*E - 3.3506;
N_mid = p1*E + 0.2494;
N_upper = p1*E + 3.8494;
%% E to W
rtk_ew_lat = cell(6,1);
rtk_ew_lon = cell(6,1);
std_ew_lat = cell(6,1);
std_ew_lon = cell(6,1);
vnd_ew_lat = cell(6,1);
vnd_ew_lon = cell(6,1);
North_rtk_ew = cell(6,1);
East_rtk_ew = cell(6,1);
North_std_ew = cell(6,1);
East_std_ew = cell(6,1);
North_vnd_ew = cell(6,1);
East_vnd_ew = cell(6,1);
st_ew = datetime([2020 12 12 21 45 40;2020 12 12 21 53 35;2020 12 12 22 01 20;...
                  2020 12 12 21 49 35;2020 12 12 21 57 05;2020 12 12 22 04 40]);
ed_ew = datetime([2020 12 12 21 46 28;2020 12 12 21 54 17;2020 12 12 22 02 10;...
                  2020 12 12 21 50 15;2020 12 12 21 57 43;2020 12 12 22 05 25]);
for i = 1:6
    rtk_st = find(RTK_t == st_ew(i));
    rtk_ed = find(RTK_t == ed_ew(i));
    std_st = find(STD_t == st_ew(i));
    std_ed = find(STD_t == ed_ew(i));
    vnd_st = find(VND_t == st_ew(i));
    vnd_ed = find(VND_t == ed_ew(i));
    North_rtk_ew{i} = NaN(1,length(rtk_st:rtk_ed));
    East_rtk_ew{i} = NaN(1,length(rtk_st:rtk_ed));
    North_std_ew{i} = NaN(1,length(std_st:std_ed));
    East_std_ew{i} = NaN(1,length(std_st:std_ed));
    North_vnd_ew{i} = NaN(1,length(vnd_st:vnd_ed));
    East_vnd_ew{i} = NaN(1,length(vnd_st:vnd_ed));
    rtk_ew_lat{i} = RTK_lat_all(rtk_st:rtk_ed);
    rtk_ew_lon{i} = RTK_lon_all(rtk_st:rtk_ed);
    std_ew_lat{i} = STD_lat_all(std_st:std_ed);
    std_ew_lon{i} = STD_lon_all(std_st:std_ed);
    vnd_ew_lat{i} = VND_lat_all(vnd_st:vnd_ed);
    vnd_ew_lon{i} = VND_lon_all(vnd_st:vnd_ed);
    for j = 1:length(rtk_st:rtk_ed)
        [North_rtk_ew{i}(j),East_rtk_ew{i}(j),zDown1] = ...
            geodetic2ned(rtk_ew_lat{i}(j),rtk_ew_lon{i}(j),330,lat0,lon0,330,wgs84);
    end
    for j = 1:length(std_st:std_ed)
        [North_std_ew{i}(j),East_std_ew{i}(j),zDown2] = ...
            geodetic2ned(std_ew_lat{i}(j),std_ew_lon{i}(j),330,lat0,lon0,330,wgs84);
    end
    for j = 1:length(vnd_st:vnd_ed)
        [North_vnd_ew{i}(j),East_vnd_ew{i}(j),zDown3] = ...
            geodetic2ned(vnd_ew_lat{i}(j),vnd_ew_lon{i}(j),330,lat0,lon0,330,wgs84);
    end
end

figure
for i = 1:6
    subplot(3,2,i)
    % Plot road side
    plot(E,N_down,'Color','k')
    hold on
    plot(E,N_mid,'Color','k')
    hold on
    plot(E,N_upper,'Color','k')
    hold on
    plot(East_rtk_ew{i},North_rtk_ew{i},'Marker','.','Color','r')
    hold on
    plot(East_std_ew{i},North_std_ew{i},'Marker','.','Color','b')
    hold on
    plot(East_vnd_ew{i},North_vnd_ew{i},'Marker','.','Color','g')
    legend('','','','RTK','SF GPS SPS','SF GPS VN')
    grid on
    xlabel('East')
    ylabel('North')
end

%% W to E
rtk_we_lat = cell(6,1);
rtk_we_lon = cell(6,1);
std_we_lat = cell(6,1);
std_we_lon = cell(6,1);
vnd_we_lat = cell(6,1);
vnd_we_lon = cell(6,1);
North_rtk_we = cell(6,1);
East_rtk_we = cell(6,1);
North_std_we = cell(6,1);
East_std_we = cell(6,1);
North_vnd_we = cell(6,1);
East_vnd_we = cell(6,1);
st_we = datetime([2020 12 12 22 03 10;2020 12 12 21 55 10;2020 12 12 21 47 40;...
                  2020 12 12 22 06 25;2020 12 12 21 58 30;2020 12 12 21 51 10]);
ed_we = datetime([2020 12 12 22 03 55;2020 12 12 21 55 51;2020 12 12 21 48 30;...
                  2020 12 12 22 07 09;2020 12 12 21 59 11;2020 12 12 21 51 52]);
for i = 1:6
    rtk_st = find(RTK_t == st_we(i));
    rtk_ed = find(RTK_t == ed_we(i));
    std_st = find(STD_t == st_we(i));
    std_ed = find(STD_t == ed_we(i));
    vnd_st = find(VND_t == st_we(i));
    vnd_ed = find(VND_t == ed_we(i));
    North_rtk_we{i} = NaN(1,length(rtk_st:rtk_ed));
    East_rtk_we{i} = NaN(1,length(rtk_st:rtk_ed));
    North_std_we{i} = NaN(1,length(std_st:std_ed));
    East_std_we{i} = NaN(1,length(std_st:std_ed));
    North_vnd_we{i} = NaN(1,length(vnd_st:vnd_ed));
    East_vnd_we{i} = NaN(1,length(vnd_st:vnd_ed));
    rtk_we_lat{i} = RTK_lat_all(rtk_st:rtk_ed);
    rtk_we_lon{i} = RTK_lon_all(rtk_st:rtk_ed);
    std_we_lat{i} = STD_lat_all(std_st:std_ed);
    std_we_lon{i} = STD_lon_all(std_st:std_ed);
    vnd_we_lat{i} = VND_lat_all(vnd_st:vnd_ed);
    vnd_we_lon{i} = VND_lon_all(vnd_st:vnd_ed);
    for j = 1:length(rtk_st:rtk_ed)
        [North_rtk_we{i}(j),East_rtk_we{i}(j),zDown1] = ...
            geodetic2ned(rtk_we_lat{i}(j),rtk_we_lon{i}(j),330,lat0,lon0,330,wgs84);
    end
    for j = 1:length(std_st:std_ed)
        [North_std_we{i}(j),East_std_we{i}(j),zDown2] = ...
            geodetic2ned(std_we_lat{i}(j),std_we_lon{i}(j),330,lat0,lon0,330,wgs84);
    end
    for j = 1:length(vnd_st:vnd_ed)
        [North_vnd_we{i}(j),East_vnd_we{i}(j),zDown3] = ...
            geodetic2ned(vnd_we_lat{i}(j),vnd_we_lon{i}(j),330,lat0,lon0,330,wgs84);
    end
end

figure
for i = 1:6
    subplot(3,2,i)
    % Plot road side
    plot(E,N_down,'Color','k')
    hold on
    plot(E,N_mid,'Color','k')
    hold on
    plot(E,N_upper,'Color','k')
    hold on
    plot(East_rtk_we{i},North_rtk_we{i},'Marker','.','Color','r')
    hold on
    plot(East_std_we{i},North_std_we{i},'Marker','.','Color','b')
    hold on
    plot(East_vnd_we{i},North_vnd_we{i},'Marker','.','Color','g')
    legend('','','','RTK','SF GPS SPS','SF GPS VN')
    grid on
    xlabel('East')
    ylabel('North')
end

%%
% p1 = 0.001617331;
% L_down = p1*E - 1.5506; (Lane center of the bottom lane)
% L_upper = p1*E + 2.0494; (Lane center of the upper lane)
mark_RTK = []; % d2down, d2upper, lane check (1 success, 0 fail)
mark_SPS = [];
mark_VND = [];
for i = 1:6
    mark_RTK = dist_comp(North_rtk_we{i},East_rtk_we{i},i,mark_RTK);
    mark_RTK = dist_comp(North_rtk_ew{i},East_rtk_ew{i},i,mark_RTK);
    mark_SPS = dist_comp(North_std_we{i},East_std_we{i},i,mark_SPS);
    mark_SPS = dist_comp(North_std_ew{i},East_std_ew{i},i,mark_SPS);
    mark_VND = dist_comp(North_vnd_we{i},East_vnd_we{i},i,mark_VND);
    mark_VND = dist_comp(North_vnd_ew{i},East_vnd_ew{i},i,mark_VND);
end

dist2center = min(mark_RTK(:,1:2),[],2);
d2l_list = 0:0.2:1.6;
x_hist = 0.1:0.2:1.5;
L = length(d2l_list);
prob_suc_VND = zeros(1,L-1);
prob_suc_SPS = zeros(1,L-1);
num_pt = zeros(1,L-1);
for j = 1:L-1
    if j == L-1
        idx = find(dist2center>=d2l_list(j) ...
                    & dist2center<=d2l_list(j+1));
    else
        idx = find(dist2center>=d2l_list(j) ....
                    & dist2center<d2l_list(j+1));
    end
    total = length(idx);
    num_pt(j) = total;
    correct1 = sum(mark_VND(idx,3));
    prob_suc_VND(j) = correct1/total;
    correct2 = sum(mark_SPS(idx,3));
    prob_suc_SPS(j) = correct2/total;
end
figure
plot(x_hist,prob_suc_VND,'-*')
hold on
plot(x_hist,prob_suc_SPS,'-*')
legend('SF GPS VN','SF GPS SPS')
ylim([0,1.1])
set(gca,'XTick',d2l_list) 
grid on
axis tight
ylim([0,1])
xlabel('Distance from lane center')
ylabel('Probability of correct lane decision')

function mark = dist_comp(North,East,i,mark)
    % i <= 3, down lane
    % i > 3, upper lane
    for j = 1: length(North)
        if ~isnan(North(j))
            d1 = d2down(East(j),North(j));
            d2 = d2upper(East(j),North(j));
            if i<=3 && d1<d2 && d1< 1.8
                check = 1;
            elseif i>3 && d1>d2 && d2< 1.8
                check = 1;
            else
                check = 0;
            end
            mark = [mark;[d1,d2,check]];
        end
    end
end


function d = d2down(x,y)
    p1 = 0.001617331;
    d = abs(p1*x - 1.5506 - y)/sqrt(p1^2+1);
end
function d = d2upper(x,y)
    p1 = 0.001617331;
    d = abs(p1*x + 2.0494 - y)/sqrt(p1^2+1);
end