clear all; close all;

Nz = 16;
Om = 64;
filename = "blep";

N = Nz*Om*2+1;

x = linspace(-Nz, Nz, N);
y = sin(pi*x)./(pi*x);
y(x == 0) = 1;
plot(x, y);
y = y .* blackman(N)';

step = sign(x);

acc = 0;
for n=1:N
  acc = acc + 1/(Nz*2) * y(n);
  BLEP(n) = acc;
end
BLEP = BLEP - 1;
BLEP_diff = BLEP - step;

figure;
## plot(x, step);
## hold on;
## plot(x, BLEP);
## hold on;
## plot(x, BLEP_diff);

full_filename = sprintf("%s_%d_%d.cpp", filename,Nz,Om);
fileid = fopen(full_filename, "w");
fprintf(fileid, "const float %s_%d_%d[] = {", filename, Nz, Om);
for n=1:N
  fprintf(fileid, "%f,", BLEP_diff(n));
end
fprintf(fileid, "};\n", BLEP_diff(n));
fclose(fileid);
