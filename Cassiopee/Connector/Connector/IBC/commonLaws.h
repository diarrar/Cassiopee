//==============================================================================
// loi log: estimation de utau
//==============================================================================
E_Float K_CONNECTOR::logf(E_Float x, E_Float a, E_Float b, E_Float c, E_Float d) 
{
  return a*log(b*x)-d/x+c;
}
// derivee
E_Float K_CONNECTOR::logfprime(E_Float x, E_Float a, E_Float d) 
{
  return a/x+d/(x*x);
}
//==============================================================================
// loi de musker : estimation de utau
//==============================================================================
E_Float K_CONNECTOR::musker(E_Float x, E_Float a, E_Float b)
{
  // double: pow(ax+10.6, 9.6) overflows float for ax > ~9000
  double ax = double(a)*x;
  double ax2 = ax*ax;
  double l1 = pow(ax+10.6,9.6);
  double l2 = ax2-8.15*ax + 86.;
  double l2s = l2*l2;
  return 5.424*atan((2.*ax-8.15)/16.7) + log10(l1/l2s) - b/x - 3.52;
}

//==============================================================================
E_Float K_CONNECTOR::muskerprime(E_Float x, E_Float a, E_Float b)
{
  double ad = a, xd = x; // double: the powers overflow float for a*x > ~9000
  double t = pow((ad*xd + 10.6),9.6);
  double tp = (ad*9.6*pow((ad*xd + 10.6),8.6) * ((ad*xd)*(ad*xd) - 8.15*ad*xd + 86.)*((ad*xd)*(ad*xd) - 8.15*ad*xd + 86.)
                - t*(2.*(ad*ad)*xd - 8.15*ad)*2.*((ad*xd)*(ad*xd) - 8.15*ad*xd + 86.))/( ((ad*xd)*(ad*xd) - 8.15*ad*xd + 86.)*((ad*xd)*(ad*xd) - 8.15*ad*xd + 86.) );
  return 5.424*2./16.7*ad/(1. + ((2.*ad*xd - 8.15)/16.7)*((2.*ad*xd - 8.15)/16.7) )+ tp/(t*log(10.)) + b/(xd*xd);
}
//==============================================================================
// fonctions pour estimer nutilde par Newton
//==============================================================================
E_Float K_CONNECTOR::fnutilde(E_Float nutilde, E_Float nut, E_Float rho, E_Float xmu)
{
  E_Float cv1cube = 7.1*7.1*7.1;   
  E_Float chi = (nutilde/xmu)*rho; 
  E_Float chicube = chi*chi*chi;
  E_Float fv1 = chicube/(chicube+cv1cube);
  return fv1*nutilde-nut;
}

E_Float K_CONNECTOR::fnutildeprime(E_Float nutilde, E_Float nut, E_Float rho, 
                                   E_Float xmu)
{
  E_Float cv1 = 7.1; 
  E_Float nutilde3 = nutilde*nutilde*nutilde;
  E_Float f = nutilde*nutilde3;//nutilde^4
  E_Float fnu = cv1*xmu/rho;
  E_Float g = nutilde3 + fnu*fnu*fnu;
  E_Float g2 = g*g;
  E_Float fp = 4*nutilde3;                              
  E_Float gp = 3*nutilde*nutilde;
  return (fp*g-f*gp)/g2;
}
