close all; clear all;

function y = squareWaveTable(f0, Fs, N)
  f1_5 = f0 * 3; % Give us a bit of headroom
  Nh = Fs / f1_5;

  Y_half = zeros([1 (N/2+1)]);

  for n=1:Nh
    if (mod(n, 2) == 0)
    else
        Y_half(n+1) = -1i * (4/(pi*n));
    endif
  end

  Y = [Y_half fliplr(conj(Y_half(2:(N/2))))];
  Y(N/2) = real(Y(N/2+1));
  Y(1) = 0;

  y = real(ifft(Y, N));
  y = y./max(abs(y));
end

function y = sawWaveTable(f0, Fs, N)
  f1_5 = f0 * 3; % Give us a bit of headroom
  Nh = Fs / f1_5;

  Y_half = zeros([1 (N/2+1)]);

  for n=1:Nh
    Y_half(n+1) = -1i * -((2*(-1)^n)/(pi*n));
  end

  Y = [Y_half fliplr(conj(Y_half(2:(N/2))))];
  Y(N/2) = real(Y(N/2+1));
  Y(1) = 0;

  y = real(ifft(Y, N));
  y = y./max(abs(y));
end

function y = triangleWaveTable(f0, Fs, N)
  f1_5 = f0 * 3; % Give us a bit of headroom
  Nh = Fs / f1_5;

  Y_half = zeros([1 (N/2+1)]);

  for n=1:Nh
    if (mod(n, 2) == 0)
    else
      k = (n+1)/2;
      Y_half(n+1) = -1i * ((8*(-1)^k)/(n^2*pi^2));
    endif
  end

  Y = [Y_half fliplr(conj(Y_half(2:(N/2))))];
  Y(N/2) = real(Y(N/2+1));
  Y(1) = 0;

  y = real(ifft(Y, N));
  y = y./max(abs(y));
end

filename="tables";
Fs = 44100;
N = 2048;
f0 = 40;
octaves = 9;

full_filename = sprintf("%s_N%d_f%d_o%d.cpp",filename,N,f0,octaves);
fileid = fopen(full_filename, "w");

%% SQUARE
square_table = [];
f = f0;
for o=1:octaves
  square_table = cat(2, square_table, squareWaveTable(f, Fs, N));
  f = f * 2;
end

fprintf(fileid, "const float square_N%d_f%d_o%d[] = {",N,f0,octaves);
for n=1:length(square_table)
  fprintf(fileid, "%f,", square_table(n));
end
fprintf(fileid, "};\n");
figure;
plot(1:(N*octaves), square_table);

%% SAW
saw_table = [];
f = f0;
for o=1:octaves
  saw_table = cat(2, saw_table, sawWaveTable(f, Fs, N));
  f = f * 2;
end

fprintf(fileid, "const float saw_N%d_f%d_o%d[] = {",N,f0,octaves);
for n=1:length(saw_table)
  fprintf(fileid, "%f,", saw_table(n));
end
fprintf(fileid, "};\n");
figure;
plot(1:(N*octaves), saw_table);

%% TRIANGLE
tri_table = [];
f = f0;
for o=1:octaves
  tri_table = cat(2, tri_table, triangleWaveTable(f, Fs, N));
  f = f * 2;
end

fprintf(fileid, "const float triangle_N%d_f%d_o%d[] = {",N,f0,octaves);
for n=1:length(saw_table)
  fprintf(fileid, "%f,", tri_table(n));
end
fprintf(fileid, "};\n");
figure;
plot(1:(N*octaves), tri_table);

fclose(fileid);

