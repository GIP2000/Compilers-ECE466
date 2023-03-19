struct s1 {
	int a;
};

void f(void)
{
	struct s1;
	struct s2 {
		struct s1 *p1;
	} s2;
	struct s3 {
		struct s2 *p2;
		struct s1 {
			double d;
		} *p1;
	} s3;
	{
		struct s1 {
			float f;
		} s1;
	}

	struct s1 s1;
}
