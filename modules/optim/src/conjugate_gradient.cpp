#include "precomp.hpp"
#undef ALEX_DEBUG
#include "debug.hpp"

namespace cv{namespace optim{

#define SEC_METHOD_ITERATIONS 4
#define INITIAL_SEC_METHOD_SIGMA 0.1
    class ConjGradSolverImpl : public ConjGradSolver
    {
    public:
        Ptr<Function> getFunction() const;
        void setFunction(const Ptr<Function>& f);
        TermCriteria getTermCriteria() const;
        ConjGradSolverImpl();
        void setTermCriteria(const TermCriteria& termcrit);
        double minimize(InputOutputArray x);
    protected:
        Ptr<Solver::Function> _Function;
        TermCriteria _termcrit;
        Mat_<double> d,r,buf_x,r_old;
        Mat_<double> minimizeOnTheLine_buf1,minimizeOnTheLine_buf2;
    private:
        static void minimizeOnTheLine(Ptr<Solver::Function> _f,Mat_<double>& x,const Mat_<double>& d,Mat_<double>& buf1,Mat_<double>& buf2);
    };

    void ConjGradSolverImpl::minimizeOnTheLine(Ptr<Solver::Function> _f,Mat_<double>& x,const Mat_<double>& d,Mat_<double>& buf1,
            Mat_<double>& buf2){
        double sigma=INITIAL_SEC_METHOD_SIGMA;
        buf1=0.0;
        buf2=0.0;

        dprintf(("before minimizeOnTheLine\n"));
        dprintf(("x:\n"));
        print_matrix(x);
        dprintf(("d:\n"));
        print_matrix(d);

        for(int i=0;i<SEC_METHOD_ITERATIONS;i++){
            _f->getGradient((double*)x.data,(double*)buf1.data);
            dprintf(("buf1:\n"));
            print_matrix(buf1);
            x=x+sigma*d;
            _f->getGradient((double*)x.data,(double*)buf2.data);
            dprintf(("buf2:\n"));
            print_matrix(buf2);
            double d1=buf1.dot(d), d2=buf2.dot(d);
            if((d1-d2)==0){
                break;
            }
            double alpha=-sigma*d1/(d2-d1);
            dprintf(("(buf2.dot(d)-buf1.dot(d))=%f\nalpha=%f\n",(buf2.dot(d)-buf1.dot(d)),alpha));
            x=x+(alpha-sigma)*d;
            sigma=-alpha;
        }

        dprintf(("after minimizeOnTheLine\n"));
        print_matrix(x);
    }

    double ConjGradSolverImpl::minimize(InputOutputArray x){
        CV_Assert(_Function.empty()==false);
        dprintf(("termcrit:\n\ttype: %d\n\tmaxCount: %d\n\tEPS: %g\n",_termcrit.type,_termcrit.maxCount,_termcrit.epsilon));

        Mat x_mat=x.getMat();
        CV_Assert(MIN(x_mat.rows,x_mat.cols)==1);
        int ndim=MAX(x_mat.rows,x_mat.cols);
        CV_Assert(x_mat.type()==CV_64FC1);

        if(d.cols!=ndim){
            d.create(1,ndim);
            r.create(1,ndim);
            r_old.create(1,ndim);
            minimizeOnTheLine_buf1.create(1,ndim);
            minimizeOnTheLine_buf2.create(1,ndim);
        }

        Mat_<double> proxy_x;
        if(x_mat.rows>1){
            buf_x.create(1,ndim);
            Mat_<double> proxy(ndim,1,(double*)buf_x.data);
            x_mat.copyTo(proxy);
            proxy_x=buf_x;
        }else{
            proxy_x=x_mat;
        }
        _Function->getGradient((double*)proxy_x.data,(double*)d.data);
        d*=-1.0;
        d.copyTo(r);

        //here everything goes. check that everything is setted properly
        dprintf(("proxy_x\n"));print_matrix(proxy_x);
        dprintf(("d first time\n"));print_matrix(d);
        dprintf(("r\n"));print_matrix(r);

        double beta=0;
        for(int count=0;count<_termcrit.maxCount;count++){
            minimizeOnTheLine(_Function,proxy_x,d,minimizeOnTheLine_buf1,minimizeOnTheLine_buf2);
            r.copyTo(r_old);
            _Function->getGradient((double*)proxy_x.data,(double*)r.data);
            r*=-1.0;
            double r_norm_sq=norm(r);
            if(_termcrit.type==(TermCriteria::MAX_ITER+TermCriteria::EPS) && r_norm_sq<_termcrit.epsilon){
                break;
            }
            r_norm_sq=r_norm_sq*r_norm_sq;
            beta=MAX(0.0,(r_norm_sq-r.dot(r_old))/r_norm_sq);
            d=r+beta*d;
        }



        if(x_mat.rows>1){
            Mat(ndim, 1, CV_64F, (double*)proxy_x.data).copyTo(x);
        }
        return _Function->calc((double*)proxy_x.data);
    }

    ConjGradSolverImpl::ConjGradSolverImpl(){
        _Function=Ptr<Function>();
    }
    Ptr<Solver::Function> ConjGradSolverImpl::getFunction()const{
        return _Function;
    }
    void ConjGradSolverImpl::setFunction(const Ptr<Function>& f){
        _Function=f;
    }
    TermCriteria ConjGradSolverImpl::getTermCriteria()const{
        return _termcrit;
    }
    void ConjGradSolverImpl::setTermCriteria(const TermCriteria& termcrit){
        CV_Assert((termcrit.type==(TermCriteria::MAX_ITER+TermCriteria::EPS) && termcrit.epsilon>0 && termcrit.maxCount>0) ||
                ((termcrit.type==TermCriteria::MAX_ITER) && termcrit.maxCount>0));
        _termcrit=termcrit;
    }
    // both minRange & minError are specified by termcrit.epsilon; In addition, user may specify the number of iterations that the algorithm does.
    Ptr<ConjGradSolver> createConjGradSolver(const Ptr<Solver::Function>& f, TermCriteria termcrit){
        ConjGradSolver *CG=new ConjGradSolverImpl();
        CG->setFunction(f);
        CG->setTermCriteria(termcrit);
        return Ptr<ConjGradSolver>(CG);
    }
}}
