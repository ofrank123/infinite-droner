%++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
% PBMMI Assignment 3 - Moog VCF Beyond the Basics 1
%
% For this beyond the basics, I have implemented the full nonlinear Moog
% VCF.  I chose to use trapezoidal integration to implement the VCF, due to
% its accuracy and stability.  I also opted to use Newton-Raphson to solve
% for the next state at each sample.  This required a fair bit of
% computation on paper to calculate the Jacobian of the function that was
% being solved.  The simulation generally takes less time per sample than
% the timestep, so it is a candidate for implementation in a realtime
% system.
%
% S1914181 O. Frank 2023-02-25
%++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

clear all; close all;

%+++++++++++++++++++++++++++++
%~ ojf: input parameters

Fs = 44.1e3;        % sample rate [Hz]
Tf = 1;             % total simulation time [s]
f0 = 2000;           % resonant filter frequency [Hz]
r  = 1;             % feedback coeff [chooses 0 \leq r \leq 1]
epsilon = 1e-15;    % precision of newton-raphson algorithm

fsaw = 110;         % saw wave input frequency
gsaw = 1;         % saw wave input gain

%+++++++++++++++++++++++++++++
%~ ojf: derived parameters

om0  = 2 * pi * f0; % angular filter frequency
Nf  = Fs * Tf;      % number of samples
k   = 1 / Fs;       % timestep per sample [s]

%+++++++++++++++++++++++++++++
%~ ojf: matrices + buffers

c = [0 0 0 1];      % output selection vector
x = zeros(4, 1);    % state
x1 = zeros(4, 1);   % state from previous sample

y = zeros(Nf, 1);   % output

tax = (0:Nf-1)'*k;
fax = (0:Nf-1)'*Fs/Nf;

%+++++++++++++++++++++++++++++
%~ ojf: input signal

u = zeros([1 Nf]);

%- ojf: fourier series to ensure the saw tooth is bandlimited
fh = fsaw;  % frequency of harmonic
kh = 1;     % harmonic #
while (fh < Fs / 2)
    b = 2 * ((-1)^(1+kh)) / (kh * pi);
    u = u + b*sin(2*pi*fh*tax');
    kh = kh + 1;
    fh = fsaw*kh;
end

%- ojf: sine wave
% u = sin(2*pi*fsaw*tax');

%- ojf: kroenecker delta
u = [1 zeros([1 Nf-1])];

%- apply input gain
u = gsaw * u;

%+++++++++++++++++++++++++++++
%~ ojf: main loop

tic
for n=1:Nf-1
    x_w1 = x;

    %- ojf: newton-raphson
    while 1
        %- ojf: f(x_n, u_n)
        fxu = om0 * [
            -tanh(x_w1(1))-tanh(4*r*x_w1(4)+u(n+1))
            -tanh(x_w1(2))+tanh(x_w1(1))
            -tanh(x_w1(3))+tanh(x_w1(2))
            -tanh(x_w1(4))+tanh(x_w1(3))
            ];

        %- ojf: f(x_n-1, u_n-1)
        fx1u1 = om0 * [
            -tanh(x1(1))-tanh(4*r*x1(4)+u(n))
            -tanh(x1(2))+tanh(x1(1))
            -tanh(x1(3))+tanh(x1(2))
            -tanh(x1(4))+tanh(x1(3))
            ];

        %- ojf: Function we are finding roots of
        G = x_w1 - x1 - (k/2)*(fxu + fx1u1);

        %- ojf: Jacobian matrix
        a = k*om0/2;
        partial_G1w1 = 1 + a*sech(x_w1(1))^2;
        partial_G1w4 = 2*k*om0*r*sech(4*r*x_w1(4)+u(n+1))^2;
        partial_G2w1 = -a*sech(x_w1(1))^2;
        partial_G2w2 = 1 + a*sech(x_w1(2))^2;
        partial_G3w2 = -a*sech(x_w1(2))^2;
        partial_G3w3 = 1 + a*sech(x_w1(3))^2;
        partial_G4w3 = -a*sech(x_w1(3))^2;
        partial_G4w4 = 1 + a*sech(x_w1(4))^2;
        J = [
            partial_G1w1 0            0            partial_G1w4
            partial_G2w1 partial_G2w2 0            0
            0            partial_G3w2 partial_G3w3 0
            0            0            partial_G4w3 partial_G4w4
            ];

        %- ojf: Next iteration of newton's method
        x_w = x_w1 - J \ G;

        %- ojf: Check if current solution is acceptable
        if ([1 1 1 1] * abs(x_w - x_w1)) <= epsilon
            x = x_w;
            break;
        end

        x_w1 = x_w;
    end

    %~ ojf: output selection
    y(n) = c * x;

    %- ojf: state update
    x1 = x;
end
simtime = toc;

%- ojf: check if code would have run at realtime
if (simtime < Tf)
    disp("Realtime acceptable!");
end

%+++++++++++++++++++++++++++++
%~ ojf: plotting + playback

figure;
Y = fft(y);
plot(fax, mag2db(abs(Y)));
xlim([10 Fs/2]);
set(gca, "XScale", "log");
title("Frequency Spectrum of Filtered Signal", "Interpreter", "latex");
xlabel("Frequency (Hz)", "Interpreter", "latex");
ylabel("Magnitude (dB)", "Interpreter", "latex");

%- ojf: remove clipping artefact
ramp = [linspace(0, 1, 50) repelem(1, length(y) - 100) linspace(1, 0, 50)]';
soundsc(y.*ramp, Fs);
