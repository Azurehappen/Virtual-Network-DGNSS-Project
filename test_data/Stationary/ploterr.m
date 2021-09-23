addpath('../')
warning off
T_SFOS = readtable('SFGPS_SPS.csv');
T_DFOS = readtable('DF_GNSSOS.csv');
T_SFVN = readtable('SFGPS_VN_SH.csv');
T_DFVN = readtable('DF_VN_SH.csv');
T_DFSBAS = readtable('DF_SBAS.csv');
grd = [-2430697.699;-4704189.201;3544329.103]; %ground truth
llh_grd = [33.9750796459;
           -117.325813435195;
           312.2889587944373];  %Degree
% Pick start time and end time to reconcile the epoch period
st = datetime([2021 6 27 14 31 51.000]);
ed = datetime([2021 6 28 02 31 51.000]);
L = seconds(ed-st)+1;
t_x = hours(0):seconds(1):hours(12);

t_SFOS = T_SFOS.UTC;
t_SFOS = datetime(t_SFOS,'InputFormat','HH:mm:ss.SSS MM/dd/yyyy');
st_ = find(t_SFOS == st);
ed_ = find(t_SFOS == ed);
SFOSx = T_SFOS.X(st_:ed_);
SFOSy = T_SFOS.Y(st_:ed_);
SFOSz = T_SFOS.Z(st_:ed_);
hor_SFOS = NaN(L,1);
ecef_SFOS = NaN(L,1);
ned_err_SFOS = NaN(3,L);
for i=1:L
    lla = ecef2lla([SFOSx(i),SFOSy(i),SFOSz(i)]);
    [hor_SFOS(i),ecef_SFOS(i),ned_err_SFOS(:,i)]=compt_err(lla(1),lla(2),lla(3),grd,llh_grd);
end

t_DFOS = T_DFOS.UTC;
t_DFOS = datetime(t_DFOS,'InputFormat','HH:mm:ss.SSS MM/dd/yyyy');
st_ = find(t_DFOS == st);
ed_ = find(t_DFOS == ed);
DFOSx = T_DFOS.X(st_:ed_);
DFOSy = T_DFOS.Y(st_:ed_);
DFOSz = T_DFOS.Z(st_:ed_);
hor_DFOS = NaN(L,1);
ecef_DFOS = NaN(L,1);
ned_err_DFOS = NaN(3,L);
for i=1:L
    lla = ecef2lla([DFOSx(i),DFOSy(i),DFOSz(i)]);
    [hor_DFOS(i),ecef_DFOS(i),ned_err_DFOS(:,i)]=...
    compt_err(lla(1),lla(2),lla(3),grd,llh_grd);
end

t_SFVN = T_SFVN.UTC;
t_SFVN = datetime(t_SFVN,'InputFormat','HH:mm:ss.SSS MM/dd/yyyy');
st_ = find(t_SFVN == st);
ed_ = find(t_SFVN == ed);
SFVNx = T_SFVN.X(st_:ed_);
SFVNy = T_SFVN.Y(st_:ed_);
SFVNz = T_SFVN.Z(st_:ed_);
hor_SFVN = NaN(L,1);
ecef_SFVN = NaN(L,1);
ned_err_SFVN = NaN(3,L);
for i=1:L
    lla = ecef2lla([SFVNx(i),SFVNy(i),SFVNz(i)]);
    [hor_SFVN(i),ecef_SFVN(i),ned_err_SFVN(:,i)]=...
    compt_err(lla(1),lla(2),lla(3),grd,llh_grd);
end

t_DFVN = T_DFVN.UTC;
t_DFVN = datetime(t_DFVN,'InputFormat','HH:mm:ss.SSS MM/dd/yyyy');
st_ = find(t_DFVN == st);
ed_ = find(t_DFVN == ed);
DFVNx = T_DFVN.X(st_:ed_);
DFVNy = T_DFVN.Y(st_:ed_);
DFVNz = T_DFVN.Z(st_:ed_);
hor_DFVN = NaN(L,1);
ecef_DFVN = NaN(L,1);
ned_err_DFVN = NaN(3,L);
for i=1:L
    lla = ecef2lla([DFVNx(i),DFVNy(i),DFVNz(i)]);
    [hor_DFVN(i),ecef_DFVN(i),ned_err_DFVN(:,i)]=...
    compt_err(lla(1),lla(2),lla(3),grd,llh_grd);
end

t_DFSBAS = T_DFSBAS.UTC;
t_DFSBAS = datetime(t_DFSBAS,'InputFormat','HH:mm:ss.SSS MM/dd/yyyy');
st_ = find(t_DFSBAS == st);
ed_ = find(t_DFSBAS == ed);
DFSBASx = T_DFSBAS.X(st_:ed_);
DFSBASy = T_DFSBAS.Y(st_:ed_);
DFSBASz = T_DFSBAS.Z(st_:ed_);
hor_DFSBAS = NaN(L,1);
ecef_DFSBAS = NaN(L,1);
ned_err_DFSBAS = NaN(3,L);
for i=1:L
    lla = ecef2lla([DFSBASx(i),DFSBASy(i),DFSBASz(i)]);
    [hor_DFSBAS(i),ecef_DFSBAS(i),ned_err_DFSBAS(:,i)]=...
    compt_err(lla(1),lla(2),lla(3),grd,llh_grd);
end

%%
purple = [0.4940, 0.1840, 0.5560];
blue = [0, 0.4470, 0.7410];
red = [0.6350, 0.0780, 0.1840];
green = [0.4660, 0.6740, 0.1880];
orange = [0.9290, 0.6940, 0.1250];

set(0,'defaultfigurecolor','w')
figure
subplot(321)
plot(t_x,hor_SFOS,'.','Color',purple)
hold on
plot(t_x,hor_DFOS,'.','Color',red)
hold on
plot(t_x,hor_SFVN,'.','Color',orange)
hold on
plot(t_x,hor_DFVN,'.','Color',blue)
hold on
plot(t_x,hor_DFSBAS,'.','Color',green)
hold on
axis tight
legend('SF GPS SPS','DF GNSS OS','SF GPS VN','SF GNSS VN','F9P SBAS')
grid on
title('(a) Horizontal Error')
xlabel('Epoch')
ylabel('Error, meter')

subplot(325)
plot(t_x,ecef_SFOS,'.','Color',purple)
hold on
plot(t_x,ecef_DFOS,'.','Color',red)
hold on
plot(t_x,ecef_SFVN,'.','Color',orange)
hold on
plot(t_x,ecef_DFVN,'.','Color',blue)
hold on
plot(t_x,ecef_DFSBAS,'.','Color',green)
hold on
legend('SF GPS SPS','DF GNSS OS','SF GPS VN','SF GNSS VN','F9P SBAS')
grid on
title('(e) 3D Error')
xlabel('Epoch')
ylabel('error, meter')

subplot(323)
plot(t_x,abs(ned_err_SFOS(3,:)),'.','Color',purple)
hold on
plot(t_x,abs(ned_err_DFOS(3,:)),'.','Color',red)
hold on
plot(t_x,abs(ned_err_SFVN(3,:)),'.','Color',orange)
hold on
plot(t_x,abs(ned_err_DFVN(3,:)),'.','Color',blue)
hold on
plot(t_x,abs(ned_err_DFSBAS(3,:)),'.','Color',green)
hold on
legend('SF GPS SPS','DF GNSS OS','SF GPS VN','SF GNSS VN','F9P SBAS')
grid on
title('(c) Vertical Error')
xlabel('Epoch');grid on;

subplot(322)
[f,x] = ecdf(hor_SFOS);
h_SFOS = semilogx(x,f); hold on;
h_SFOS.Color = purple;
h_SFOS.Marker = 'o';
h_SFOS.LineWidth = 0.2;
h_SFOS.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-1))==abs(x-1));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',purple,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k')
%datatip(h_SFOS,x(ind),f(ind));

[f,x] = ecdf(hor_DFOS);
h_DFOS = semilogx(x,f); hold on;
h_DFOS.Color = red;
h_DFOS.Marker = '>';
h_DFOS.LineWidth = 0.2;
h_DFOS.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-1))==abs(x-1));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',red,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k')
%datatip(h_DFOS,x(ind),f(ind));

[f,x] = ecdf(hor_DFSBAS);
h_DFSBAS = semilogx(x,f); hold on;
h_DFSBAS.Color = green;
h_DFSBAS.Marker = '>';
h_DFSBAS.LineWidth = 0.2;
h_DFSBAS.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-1))==abs(x-1));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',green,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k')
%datatip(h_DFSBAS,x(ind),f(ind));

line([1,1],[f(1) f(ind)],'linestyle',':','color','k','LineWidth',2);
line([1.5,1.5],[f(1) 1],'linestyle',':','color','k','LineWidth',2);

[f,x] = ecdf(hor_DFVN);
h_DFVN = semilogx(x,f); hold on;
h_DFVN.Color = blue;
h_DFVN.Marker = '>';
h_DFVN.LineWidth = 0.2;
h_DFVN.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-1))==abs(x-1));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',blue,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k')
%datatip(h_DFVN,x(ind),f(ind));

[f,x] = ecdf(hor_SFVN);
h_SFVN = semilogx(x,f); hold on;
h_SFVN.Color = orange;
h_SFVN.Marker = '>';
h_SFVN.LineWidth = 0.2;
h_SFVN.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-1))==abs(x-1));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',orange,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k')
%datatip(h_SFVN,x(ind),f(ind));

legend([h_SFOS,h_DFOS,h_SFVN,h_DFVN,h_DFSBAS],...
    {'SF GPS SPS','DF GNSS OS','SF GPS VN','SF GNSS VN','F9P SBAS'})
legend('location','southeast');
title('(b) Cumulative Distribution of Horizontal Error');
xlabel('Accuracy, meter');
ylabel('Cumulative Probability');
xlim([0.01 10]);
set(gca,'xtick',[0.01,0.1,1,1.5,3,10]);
set(gca,'xticklabel',{'0.01','0.1','1','1.5','3','10'});
grid on

% Cumulative Distribution of Vertical Error
subplot(324)
[f,x] = ecdf(abs(ned_err_SFOS(3,:)));
h_SFOS = semilogx(x,f); hold on;
h_SFOS.Color = purple;
h_SFOS.Marker = 'o';
h_SFOS.LineWidth = 0.2;
h_SFOS.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-3))==abs(x-3));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',purple,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k'); hold on;
%datatip(h_SFOS,x(ind),f(ind));

[f,x] = ecdf(abs(ned_err_DFOS(3,:)));
h_DFOS = semilogx(x,f); hold on;
h_DFOS.Color = red;
h_DFOS.Marker = '>';
h_DFOS.LineWidth = 0.2;
h_DFOS.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-3))==abs(x-3));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',red,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k'); hold on;
%datatip(h_DFOS,x(ind),f(ind));

[f,x] = ecdf(abs(ned_err_DFSBAS(3,:)));
h_DFSBAS = semilogx(x,f); hold on;
h_DFSBAS.Color = green;
h_DFSBAS.Marker = '>';
h_DFSBAS.LineWidth = 0.2;
h_DFSBAS.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-3))==abs(x-3));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',green,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k'); hold on;
%datatip(h_DFSBAS,x(ind),f(ind));

line([3,3],[f(1) f(ind)],'linestyle',':','color','k','LineWidth',2);

[f,x] = ecdf(abs(ned_err_DFVN(3,:)));
h_DFVN = semilogx(x,f); hold on;
h_DFVN.Color = blue;
h_DFVN.Marker = '>';
h_DFVN.LineWidth = 0.2;
h_DFVN.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-3))==abs(x-3));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',blue,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k'); hold on;
%datatip(h_DFVN,x(ind),f(ind));

[f,x] = ecdf(abs(ned_err_SFVN(3,:)));
h_SFVN = semilogx(x,f); hold on;
h_SFVN.Color = orange;
h_SFVN.Marker = '>';
h_SFVN.LineWidth = 0.2;
h_SFVN.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-3))==abs(x-3));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',orange,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k'); hold on;
%datatip(h_SFVN,x(ind),f(ind));

legend([h_SFOS,h_DFOS,h_SFVN,h_DFVN,h_DFSBAS],...
    {'SF GPS SPS','DF GNSS OS','SF GPS VN','SF GNSS VN','F9P SBAS'})
legend('location','best');
title('(d) Cumulative Distribution of Vertical Error');
xlabel('Accuracy, meter');
ylabel('Cumulative Probability');
xlim([0.01 10]);
set(gca,'xtick',[0.01,0.1,1,3,10]);
set(gca,'xticklabel',{'0.01','0.1','1','3','10'});
grid on

% Cumulative Distribution of 3D Error
subplot(326)
[f,x] = ecdf(ecef_SFOS);
h_SFOS = semilogx(x,f); hold on;
h_SFOS.Color = purple;
h_SFOS.Marker = 'o';
h_SFOS.LineWidth = 0.2;
h_SFOS.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-3))==abs(x-3));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',purple,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k'); hold on;
%datatip(h_SFOS,x(ind),f(ind));

[f,x] = ecdf(ecef_DFOS);
h_DFOS = semilogx(x,f); hold on;
h_DFOS.Color = red;
h_DFOS.Marker = '>';
h_DFOS.LineWidth = 0.2;
h_DFOS.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-3))==abs(x-3));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',red,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k'); hold on;
%datatip(h_DFOS,x(ind),f(ind));

[f,x] = ecdf(ecef_DFSBAS);
h_DFSBAS = semilogx(x,f); hold on;
h_DFSBAS.Color = green;
h_DFSBAS.Marker = '>';
h_DFSBAS.LineWidth = 0.2;
h_DFSBAS.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-3))==abs(x-3));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',green,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k'); hold on;
%datatip(h_DFSBAS,x(ind),f(ind));

line([3,3],[f(1) f(ind)],'linestyle',':','color','k','LineWidth',2);

[f,x] = ecdf(ecef_DFVN);
h_DFVN = semilogx(x,f); hold on;
h_DFVN.Color = blue;
h_DFVN.Marker = '>';
h_DFVN.LineWidth = 0.2;
h_DFVN.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-3))==abs(x-3));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',blue,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k'); hold on;
%datatip(h_DFVN,x(ind),f(ind));

[f,x] = ecdf(ecef_SFVN);
h_SFVN = semilogx(x,f); hold on;
h_SFVN.Color = orange;
h_SFVN.Marker = '>';
h_SFVN.LineWidth = 0.2;
h_SFVN.MarkerIndices = 1:2000:L;
ind = find(min(abs(x-3))==abs(x-3));
ind = ind(1);
line([0.01,x(ind)],[f(ind) f(ind)],'linestyle',':','color',orange,'LineWidth',1.5);
scatter(x(ind),f(ind),'filled','k');
%datatip(h_SFVN,x(ind),f(ind));

legend([h_SFOS,h_DFOS,h_SFVN,h_DFVN,h_DFSBAS],...
    {'SF GPS SPS','DF GNSS OS','SF GPS VN','SF GNSS VN','F9P SBAS'})
legend('location','best');
title('(f) Cumulative Distribution of 3D Error');
xlabel('Accuracy, meter');
ylabel('Cumulative Probability');
xlim([0.01 10]);
set(gca,'xtick',[0.01,0.1,1,3,10]);
set(gca,'xticklabel',{'0.01','0.1','1','3','10'});
grid on

%% Compute percentage
T = abs(ned_err_SFOS(3,:));
per = length(find(T<=3.0))/length(T)

T = abs(ned_err_DFOS(3,:));
per = length(find(T<=3.0))/length(T)

T = abs(ned_err_DFSBAS(3,:));
per = length(find(T<=3.0))/length(T)

T = abs(ned_err_DFVN(3,:));
per = length(find(T<=3.0))/length(T)

T = abs(ned_err_SFVN(3,:));
per = length(find(T<=3.0))/length(T)

%
T = ecef_SFOS;
per = length(find(T<=3.0))/length(T)

T = ecef_DFOS;
per = length(find(T<=3.0))/length(T)

T = ecef_DFSBAS;
per = length(find(T<=3.0))/length(T)

T = ecef_DFVN;
per = length(find(T<=3.0))/length(T)

T = ecef_SFVN;
per = length(find(T<=3.0))/length(T)

%
T = hor_SFOS;
per = length(find(T<=1.5))/length(T)

T = hor_DFOS;
per = length(find(T<=1.5))/length(T)

T = hor_DFSBAS;
per = length(find(T<=1.5))/length(T)

T = hor_DFVN;
per = length(find(T<=1.5))/length(T)

T = hor_SFVN;
per = length(find(T<=1.5))/length(T)