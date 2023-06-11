#align(center, text(17pt)[*Combining WDF Elements*])

With WDFs, it's pretty common to combine a series
voltage source plus resistor into a "resistive voltage 
source", in order to be able to use a voltage source
as an "adaptable" WDF one-port. However, we can also
combine other WDF elements with the goal being that
the C++ compiler can better optimize the computations
needed for the combined elements.

= Resistor-Capacitor Series
Here we create a combined one-port for a resistor and
capacitor in series. Note that with the combined element
we no longer have direct access to the voltage or current
across either the resistor or capacitor, so this approach
doesn't work great if you need to "probe" either element.

== Underlying Components
A WDF Resistor is defined by:

$ R_p = R, b = 0 $

A WDF Capacitor is defined by:

$ R_p = T/(2C), b = a[n-1] $

And a 3-port WDF Series adaptor:

$
R_p = R_1 + R_2 \
b_0 = -a_1 - a_2 \
b_1 = -R_1/R_p a_0
    + R_2/R_p a_1
    - R_1/R_p a_2 \
b_2 = -R_2/R_p a_0
    - R_2/R_p a_1
    + R_1/R_p a_2
$

We can simplify this into 1-multiply form. First, we
recognize the similarity between the $b_1$ and $b_2$ terms:
$
b_1 + b_2 = -R_1/R_p a_0 - R_2/R_p a_0 \
b_1 + b_2 = -((R_1 + R_2)/R_p a_0)
$

Recall that $R_p = R_1 + R_2$, so:
$
b_1 + b_2 = -((R_1 + R_2)/(R_1 + R_2) a_0) \
b_1 + b_2 = -a_0 \
b_2 = -(a_0 + b_1)
$

So if we can figure out $b_1$ then we can compute $b_2$
with zero multiplications.

$
b_1 = -R_1/R_p a_0 + R_2/R_p a_1 - R_1/R_p a_2 \
b_1 = -R_1/R_p a_0 + R_2/R_p a_1 - R_1/R_p a_2
    + R_1/R_p a_1 - R_1/R_p a_1 \
b_1 = -R_1/R_p (a_0 + a_1 + a_2) + R_2/R_p a_1 + R_1/R_p a_1 \
b_1 = a_1 -R_1/R_p (a_0 + a_1 + a_2)
$

So the full 1-multiply series adaptor is:
$
R_p = R_1 + R_2 \
b_0 = -a_1 - a_2 \
b_1 = a_1 -R_1/R_p (a_0 + a_1 + a_2) \
b_2 = -(a_0 + b_1)
$

== Combining The Elements

Now we can combine these elements. We'll use port (1)
of the series adaptor for the capacitor, and port (2)
for the resistor. Remember that $a$ and $b$ are being
described from the reference point of the given element,
so $a_1$ for the series adaptor is actually $b$ from the 
capacitor. To avoid duplicated names, we'll use $b_c$ and
$a_c$ for the variables from the perspective of the capacitor
and $b_r$ and $a_r$ for the resistor. In other words:
$a_1 := b_c$, $b_1 := a_c$, and so on.

First we sub in $b_c$ and $b_r$ for $a_1$ and $a_2$:
$
R_p = T/(2C) + R \
R_i := R_1/R_p = (T/(2C))/(T/(2C) + R)
    = T/(T + 2 R C) \
b_0 = -b_c - b_r \
b_1 = a_c[n-1] - R_i (a_0 + b_c + b_r) \
b_2 = -(a_0 + b_1)
$

And then we can sub in the definitions $b_c = a_c[n-1]$ and $b_r = 0$:
$
b_0 = -a_c[n-1] - 0 \
b_1 = a_c[n-1] - R_i (a_0 + a_c[n-1] + 0) \
b_2 = -(a_0 + b_1)
$

Now we can sub in $a_c$ and $a_r$ for $b_1$ and $b_2$:
$
b_0 = -a_c[n-1] \
a_c = a_c[n-1] - R_i (a_0 + a_c[n-1]) \
a_r = -(a_0 + a_c)
$

Since the only thing we need to compute is $b_0$, and
$b_0$ only depends on $a_c$, we no longer need to compute 
$a_r$, and we can rename $a_c$ to $z$ (since that's 
essentially the "state" for this one-port).
$
b_0 = -z[n-1] \
z = z[n-1] - R_i (a_0 + z[n-1])
$

#show link: underline
So what's the advantage here? The series adaptor
needs roughly 7 additions/subtractions and 1 multiply.
The combined element needs only 3 additions/subtractions
(and still the 1 multiply), but we've also removed a
layer of complexity that we were hoping the compiler
would optimize through. This results in better generated
assembly (see, e.g.,
#link("https://godbolt.org/z/Esodbobh3")), and
probably faster compile times as well. For WDF trees that
are very deep, simplifying these elements also makes it
less likely that the compiler will "give up" part-way
through the optimizing process because of time/memory
constraints.

= Resistor-Capacitor Parallel

== Underlying Components

The 3-port parallel adaptor is defined:
$
G_p = G_1 + G_2 \
b_0 = G_1/G_p a_1 + G_2/G_p a_2 \
b_1 = a_0 - G_2/G_p a_1 + G_2/G_p a_2 \
b_2 = a_0 + G_2/G_p a_1 - G_2/G_p a_2
$

Again, we find a simple relationship between $b_1$ and $b_2$:
$
b_d := a_2 - a_1 \
b_1 = a_0 + G_2/G_p b_d \
b_2 = a_0 - G_1/G_p b_d \
b_1 - b_2 = G_2/G_p b_d+G_1/G_p b_d \
b_1 - b_2 = (G_1 + G_2)/G_p b_d \
b_1 - b_2 = (G_1 + G_2)/(G_1 + G_2) b_d \
b_1 - b_2 = b_d
$

Then solving for $b_0$:
$
b_0 = G_1/G_p a_1
    + G_2/G_p a_2
    + G_1/G_p a_2
    - G_1/G_p a_2 \
b_0 = G_1/G_p a_2 + G_2/G_p a_2
    + G_1/G_p (a_1 - a_2) \
b_0 = a_2 - G_1/G_p b_d
$

So the full 1-multiply parallel adaptor:
$
G_p = G_1 + G_2 \
b_d = a_2 - a_1 \
b_0 = a_2 - G_1/G_p b_d \
b_1 = b_2 + b_d \
b_2 = a_0 - G_1/G_p b_d = b_0 - a_2 + a_0
$

== Combining The Elements

Again, we can combine the elements with the capacitor
at port (1), and resistor at port (2):

$
G_p = (2C)/T + 1/R \
G_i := G_1/G_p = ((2C)/T)/G_p = (2 C R)/(2 C R + T) \
b_d = b_r - b_c \
b_0 = b_r - G_i b_d \
b_1 = b_2 + b_d \
b_2 = b_0 - b_r + a_0
$

$b_d$ condenses very nicely, to $-a_c[n-1]$, so:

$
b_0 = 0 + G_i a_c[n-1] \
b_1 = b_2 - a_c[n-1] \
b_2 = b_0 - 0 + a_0
$

Then substituting in $b_1 -> a_c$ and $b_2 -> a_r$:
$
b_0 = G_i a_c[n-1] \
a_c = a_r - a_c[n-1] \
a_r = b_0 + a_0 \
$

Substituing in for $a_r$, we get:
$
a_c = b_0 + a_0 - a_c[n-1]
$

So the final derivation:
$
b_0 = G_i z[n-1] \
z = b_0 + a_0 - z[n-1]
$

= Resistive Voltage Source + Capacitor (in Series)

== Underlying Components

A WDF Resistive Voltage source is defined:

$ R_p = R, b = V $

== Combining The Elements

Now we can start again with the 1-multiply series adaptor:

$
R_p = R_1 + R_2 \
b_0 = -a_1 - a_2 \
b_1 = a_1 - R_1/R_p (a_0 + a_1 + a_2) \
b_2 = -(a_0 + b_1)
$

And start subbing in:
$
R_p = T/(2C) + R \
R_i := R_1/R_p = T/(T + 2 R C) \
b_0 = -a_c[n-1] - V \
a_c = a_c[n-1] - R_i (a_0 + a_c[n-1] + V)
$

Note that as with the previous series combination,
we can basically ignore the $a_r$ term. We can simplify
$a_c$ a little bit more by subbing in the definition of
$b_0$:
$
a_c = a_c[n-1] - R_i (a_0 - b_0)
$

In final form:
$
b_0 = -z[n-1] - V \
z = z[n-1] - R_i (a_0 - b_0)
$

= Capacitive Voltage Source (Series)

Let's start with the Kirchoff domain definition
for a Resistive Voltage Source:
$ v(t) - e(t) = i(t) R $

Now we replace the resistor term with a capacitor,
and switch to the $s$-domain:
$ V(s) - E(s) = I(s) / (C s) $

And then moving to the wave domain:
$ 1/2 (A(s) + B(s)) - E(s) = 1/(2 R_0) (A(s) - B(s))/(C s) $
$ R_0 C s (A (s) + B(s)) - 2 R_0 C s E(s) = A(s) - B(s) $
$ B(s) (R_0 C s + 1) = A(s) (1 - R_0 C s) + 2 R_0 C s E(s) $
$ B(s) = A(s) (1 - R_0 C s)/(1 + R_0 C s)
       + E(s) (2 R_0 C s)/(1 + R_0 C s) $

Now we apply the bilinear transform: $s -> 2/T (1 - z^(-1))/(1 + z^(-1))$:
$
B(z) = A(z) (1 - R_0 C 2/T (1 - z^(-1))
     / (1 + z^(-1)))/(1 + R_0 C 2/T (1 - z^(-1))/(1 + z^(-1)))
     + E(z) (2 R_0 C 2/T (1 - z^(-1))/(1 + z^(-1)))
     / (1 + R_0 C 2/T (1 - z^(-1))/(1 + z^(-1)))
$

Multiplying through by $(T (1 + z^(-1)))/(T (1 + z^(-1)))$:
$
B(z) = A(z) (T (1 + z^(-1)) - 2 R_0 C (1 - z^(-1)))
     / (T (1 + z^(-1)) + 2 R_0 C (1 - z^(-1)))
     + E(z) (4 R_0 C (1 - z^(-1)))
     / (T (1 + z^(-1)) + 2 R_0 C (1 - z^(-1)))
$
$
B(z) = A(z) ((T - 2 R_0 C) + (T + 2 R_0 C)z^(-1))
     / ((T + 2 R_0 C) + (T - 2 R_0 C)z^(-1))
     + E(z) (4 R_0 C - 4 R_0 C z^(-1))
     / ((T + 2 R_0 C) + (T - 2 R_0 C)z^(-1))
$

Now we can adapt the port by setting $R_0 = T/(2 C)$ (note 
that this is the same port impedance as a singular capacitor).
We can then simplify $2 R_0 C -> T$
$ B(z) = A(z) (2 T z^(-1))/(2 T) + E(z) (2 - 2 z^(-1))/(2T) $
$ B(z) = A(z) z^(-1) + E(z) (1 - z^(-1)) $

Applying the inverse $z$-transform:
$ b[n] = a[n-1] + e[n] - e[n-1] $
